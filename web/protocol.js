// UlpanIR BLE configuration protocol v1 (docs/ble_protocol.md): pure logic, no DOM, no Bluetooth.
// Used by index.html and tested with `node --test web/test`.

const uuid = (n) => `d5e8000${n}-b2f6-4b31-98c6-7923e11860fc`;
export const UUID = {
  service: uuid(1),
  config: uuid(2),
  status: uuid(3),
  ping: uuid(4),
  test: uuid(5),
};

export const BLOCK_LEN = 11;
export const VERSION = 1;
export const REPEAT_HZ_MIN = 1;
export const REPEAT_HZ_MAX = 10;

// The remote answers only "ok" or "rejected" (0x80); the page checks the values itself and names the reason,
// the remote writes its reason to the developer log (UART).
export const DEVICE_REJECTED = 0x80;

export function errorText() {
  return 'Пульт не принял данные. Проверьте значения и повторите; если не помогло — версии страницы и прошивки не совпадают';
}

/** Defaults of a clean remote: nothing assigned, 3 frames/s, full frame. */
export function defaults() {
  const none = { assigned: false, extended: false, address: 0, command: 0 };
  return { a: { ...none }, b: { ...none }, repeatHz: 3, repeatMode: 0 };
}

/** The firmware checks (settings_parse) plus form input, with the reason in words; null if valid. */
export function validate(s) {
  for (const k of ['a', 'b']) {
    const c = s[k], name = `Кнопка ${k.toUpperCase()}`;
    if (!c.assigned) continue;
    if (!Number.isInteger(c.address) || !Number.isInteger(c.command))
      return `${name}: адрес и команда — шестнадцатеричные числа, например 0x32`;
    if (!c.extended && c.address > 0xff) return `${name}: адрес больше 0xFF — включите «16 бит» или исправьте адрес`;
    if (c.address < 0 || c.address > 0xffff) return `${name}: адрес — от 0x0000 до 0xFFFF`;
    if (c.command < 0 || c.command > 0xff) return `${name}: команда — от 0x00 до 0xFF`;
  }
  if (!Number.isInteger(s.repeatHz) || s.repeatHz < REPEAT_HZ_MIN || s.repeatHz > REPEAT_HZ_MAX)
    return 'Частота повторов — от 1 до 10 кадров/с';
  if (s.repeatMode !== 0 && s.repeatMode !== 1) return 'Вид повтора — «полный кадр» или «код повтора NEC»';
  return null;
}

function encodeButton(c, out, off) {
  out[off] = (c.assigned ? 1 : 0) | (c.extended ? 2 : 0);
  out[off + 1] = c.address & 0xff;
  out[off + 2] = (c.address >> 8) & 0xff;
  out[off + 3] = c.command & 0xff;
}

export function encodeBlock(s) {
  const out = new Uint8Array(BLOCK_LEN);
  out[0] = VERSION;
  encodeButton(s.a, out, 1);
  encodeButton(s.b, out, 5);
  out[9] = s.repeatHz;
  out[10] = s.repeatMode;
  return out;
}

function decodeButton(b, off) {
  return {
    assigned: (b[off] & 1) !== 0,
    extended: (b[off] & 2) !== 0,
    address: b[off + 1] | (b[off + 2] << 8),
    command: b[off + 3],
  };
}

/** Decodes a block read from the remote; throws on a block this page does not understand. */
export function decodeBlock(bytes) {
  const b = bytes instanceof DataView ? new Uint8Array(bytes.buffer, bytes.byteOffset, bytes.byteLength) : bytes;
  if (b.length !== BLOCK_LEN) throw new Error('Неверная длина настроек: версии страницы и прошивки не совпадают');
  if (b[0] !== VERSION) throw new Error('Пульт хранит другую версию настроек: обновите страницу или прошивку');
  return { a: decodeButton(b, 1), b: decodeButton(b, 5), repeatHz: b[9], repeatMode: b[10] };
}

export function decodeStatus(bytes) {
  const b = bytes instanceof DataView ? new Uint8Array(bytes.buffer, bytes.byteOffset, bytes.byteLength) : bytes;
  if (b.length < 6) throw new Error('Неверная длина статуса: версии страницы и прошивки не совпадают');
  return { proto: b[0], firmware: `${b[1]}.${b[2]}.${b[3]}`, batteryV: (b[4] | (b[5] << 8)) / 1000 };
}

/** `test` payload: button (0 = A, 1 = B) + the block from the form, so an unsaved code can be checked. */
export function testPayload(button, s) {
  const out = new Uint8Array(1 + BLOCK_LEN);
  out[0] = button;
  out.set(encodeBlock(s), 1);
  return out;
}

/** The 4 NEC data bytes sent for a button: address, ~address (or 16-bit address), command, ~command. */
export function necBytes(c) {
  if (!c.assigned) return null;
  const a = c.extended ? [c.address & 0xff, (c.address >> 8) & 0xff] : [c.address & 0xff, ~c.address & 0xff];
  return [...a, c.command & 0xff, ~c.command & 0xff];
}

export const hex = (bytes) => Array.from(bytes, (x) => x.toString(16).padStart(2, '0').toUpperCase()).join(' ');

/** Parses "0x32", "32", "0X1234" as hex; returns NaN for anything else. */
export function parseHex(text) {
  const t = String(text).trim().replace(/^0x/i, '');
  return /^[0-9a-f]{1,4}$/i.test(t) ? parseInt(t, 16) : NaN;
}

/**
 * Session timers (spec "Пинг со страницы", "Предупреждение о долгой сессии"):
 * ping every 30 s; 9 min after connect (or after "Продолжить") ask; no answer within 60 s -> disconnect.
 * tick(now) returns 'ping' | 'warn' | 'disconnect' | null; the page acts on it.
 */
export const TIMERS = { pingMs: 30_000, warnAfterMs: 9 * 60_000, answerMs: 60_000 };

export class Session {
  constructor(timers = TIMERS) {
    this.t = timers;
    this.active = false;
  }
  start(now) {
    this.active = true;
    this.lastPing = now;
    this.periodStart = now;
    this.warnedAt = null;
  }
  continue(now) {
    this.periodStart = now;
    this.warnedAt = null;
  }
  stop() {
    this.active = false;
  }
  tick(now) {
    if (!this.active) return null;
    if (this.warnedAt !== null) {
      if (now - this.warnedAt >= this.t.answerMs) {
        this.active = false;
        return 'disconnect';
      }
    } else if (now - this.periodStart >= this.t.warnAfterMs) {
      this.warnedAt = now;
      return 'warn';
    }
    if (now - this.lastPing >= this.t.pingMs) {
      this.lastPing = now;
      return 'ping';
    }
    return null;
  }
}
