// node --test web/test
import { test } from 'node:test';
import assert from 'node:assert/strict';
import {
  UUID, DEVICE_REJECTED, defaults, validate, encodeBlock, decodeBlock, decodeStatus, testPayload, necBytes, hex, parseHex,
  Session,
} from '../protocol.js';
import { STRINGS, LANGS, DEFAULT_LANG, t } from '../i18n.js';

// Example from docs/ble_protocol.md: A = 0x32/0x02, B = 0x32/0x03, 3 frames/s, full frame.
const EXAMPLE = [0x01, 0x01, 0x32, 0x00, 0x02, 0x01, 0x32, 0x00, 0x03, 0x03, 0x00];
const exampleSettings = () => ({
  a: { assigned: true, extended: false, address: 0x32, command: 0x02 },
  b: { assigned: true, extended: false, address: 0x32, command: 0x03 },
  repeatHz: 3,
  repeatMode: 0,
});

test('UUIDs match docs/ble_protocol.md', () => {
  assert.equal(UUID.service, 'd5e80001-b2f6-4b31-98c6-7923e11860fc');
  assert.equal(UUID.test, 'd5e80005-b2f6-4b31-98c6-7923e11860fc');
});

test('encode: documented example bytes', () => {
  assert.deepEqual([...encodeBlock(exampleSettings())], EXAMPLE);
});

test('decode: example and 16-bit address round trip', () => {
  assert.deepEqual(decodeBlock(Uint8Array.from(EXAMPLE)), exampleSettings());
  const s = defaults();
  s.a = { assigned: true, extended: true, address: 0xbeef, command: 0x7a };
  s.repeatHz = 10;
  s.repeatMode = 1;
  const bytes = encodeBlock(s);
  assert.deepEqual([...bytes.slice(1, 5)], [0x03, 0xef, 0xbe, 0x7a]);
  assert.deepEqual(decodeBlock(bytes), s);
});

test('decode: DataView from Web Bluetooth', () => {
  const dv = new DataView(Uint8Array.from(EXAMPLE).buffer);
  assert.equal(decodeBlock(dv).b.command, 0x03);
});

test('decode: wrong version or length is rejected', () => {
  assert.throws(() => decodeBlock(Uint8Array.from([2, ...EXAMPLE.slice(1)])), { message: 'errBlockVersion' });
  assert.throws(() => decodeBlock(Uint8Array.from(EXAMPLE.slice(0, 10))), { message: 'errBlockLength' });
});

test('validate: spec scenario, 50 frames/s is not sent', () => {
  const s = exampleSettings();
  s.repeatHz = 50;
  assert.deepEqual(validate(s), { key: 'errRepeatHz', params: {} });
  assert.equal(t('ru', 'errRepeatHz'), 'Частота повторов — от 1 до 10 кадров/с');
});

test('validate: ranges', () => {
  const s = exampleSettings();
  assert.equal(validate(s), null);
  s.repeatHz = 0;
  assert.equal(validate(s).key, 'errRepeatHz');
  s.repeatHz = 10;
  assert.equal(validate(s), null);
  s.a.address = 0x132; // 8-bit address too large
  assert.deepEqual(validate(s), { key: 'errAddress8', params: { button: 'A' } });
  s.a.extended = true;
  assert.equal(validate(s), null);
  s.a.command = 0x100;
  assert.equal(validate(s).key, 'errCommand');
  s.a.command = 0x10;
  s.b.address = NaN; // not a hex number in the form
  assert.deepEqual(validate(s), { key: 'errHex', params: { button: 'B' } });
  s.b.address = 0x32;
  s.repeatMode = 2;
  assert.equal(validate(s).key, 'errRepeatMode');
});

test('validate: unassigned button fields are not checked', () => {
  const s = exampleSettings();
  s.b = { assigned: false, extended: false, address: NaN, command: NaN };
  assert.equal(validate(s), null);
});

test('device: one rejection code', () => {
  assert.equal(DEVICE_REJECTED, 0x80);
});

test('i18n: English by default, Russian available', () => {
  assert.equal(DEFAULT_LANG, 'en');
  assert.deepEqual(Object.keys(LANGS), ['en', 'ru']);
  assert.equal(t('en', 'connect'), 'Connect');
  assert.equal(t('ru', 'connect'), 'Подключить');
});

test('i18n: every language has every key with the same placeholders', () => {
  const ph = (s) => [...s.matchAll(/\{(\w+)\}/g)].map((m) => m[1]).sort().join(',');
  const en = STRINGS.en;
  for (const lang of Object.keys(LANGS)) {
    assert.deepEqual(Object.keys(STRINGS[lang]).sort(), Object.keys(en).sort(), lang);
    for (const k of Object.keys(en)) assert.equal(ph(STRINGS[lang][k]), ph(en[k]), `${lang}.${k}`);
  }
});

test('i18n: placeholders, fallback to English', () => {
  assert.equal(t('en', 'codeSent', { button: 'B' }), 'Code B sent');
  assert.equal(t('ru', 'errHex', { button: 'A' }).startsWith('Кнопка A:'), true);
  assert.equal(t('de', 'connect'), 'Connect');
  assert.equal(t('en', 'noSuchKey'), 'noSuchKey');
});

test('i18n: every key used by validate exists', () => {
  for (const k of ['errHex', 'errAddress8', 'errAddress16', 'errCommand', 'errRepeatHz', 'errRepeatMode',
    'errBlockLength', 'errBlockVersion', 'errStatusLength']) assert.ok(k in STRINGS.en, k);
});

test('status: version and battery volts', () => {
  assert.deepEqual(decodeStatus(Uint8Array.from([1, 0, 2, 0, 0x39, 0x0b])), {
    proto: 1, firmware: '0.2.0', batteryV: 2.873,
  });
});

test('test payload: button + unsaved block', () => {
  const p = testPayload(1, exampleSettings());
  assert.equal(p.length, 12);
  assert.equal(p[0], 1);
  assert.deepEqual([...p.slice(1)], EXAMPLE);
});

test('NEC byte preview', () => {
  assert.equal(hex(necBytes({ assigned: true, extended: false, address: 0x32, command: 0x02 })), '32 CD 02 FD');
  assert.equal(hex(necBytes({ assigned: true, extended: true, address: 0x1234, command: 0x02 })), '34 12 02 FD');
  assert.equal(necBytes({ assigned: false, extended: false, address: 0, command: 0 }), null);
});

test('parseHex', () => {
  assert.equal(parseHex('0x32'), 0x32);
  assert.equal(parseHex(' 1234 '), 0x1234);
  assert.ok(Number.isNaN(parseHex('xyz')));
  assert.ok(Number.isNaN(parseHex('12345')));
  assert.ok(Number.isNaN(parseHex('')));
});

// Session timers: run a fake clock in 1 s steps and collect what the page is told to do.
function run(session, from, to, log) {
  for (let t = from; t <= to; t += 1000) {
    const a = session.tick(t);
    if (a) log.push([a, t]);
  }
}

test('session: ping every 30 s, user idle 5 min keeps the connection', () => {
  const s = new Session();
  const log = [];
  s.start(0);
  run(s, 0, 5 * 60_000, log);
  assert.equal(log.filter(([a]) => a === 'ping').length, 10);
  assert.ok(!log.some(([a]) => a === 'disconnect'));
});

test('session: warning after 9 min, "Продолжить" restarts the 9 min', () => {
  const s = new Session();
  const log = [];
  s.start(0);
  run(s, 0, 9 * 60_000, log);
  assert.deepEqual(log.filter(([a]) => a === 'warn'), [['warn', 9 * 60_000]]);
  s.continue(9 * 60_000 + 5000);
  const log2 = [];
  run(s, 9 * 60_000 + 6000, 18 * 60_000 + 4000, log2);
  assert.ok(!log2.some(([a]) => a === 'warn' || a === 'disconnect'));
  run(s, 18 * 60_000 + 5000, 18 * 60_000 + 5000, log2);
  assert.deepEqual(log2.at(-1), ['warn', 18 * 60_000 + 5000]);
});

test('session: no answer for 60 s -> disconnect, then nothing more', () => {
  const s = new Session();
  const log = [];
  s.start(0);
  run(s, 0, 11 * 60_000, log);
  const warn = log.find(([a]) => a === 'warn')[1];
  const disc = log.find(([a]) => a === 'disconnect')[1];
  assert.equal(disc - warn, 60_000);
  assert.ok(!log.some(([, t]) => t > disc)); // no ping after disconnect
});

test('session: ping continues while the warning is shown', () => {
  const s = new Session();
  const log = [];
  s.start(0);
  run(s, 0, 10 * 60_000, log);
  const warn = log.find(([a]) => a === 'warn')[1];
  assert.ok(log.some(([a, t]) => a === 'ping' && t > warn));
});

test('session: accelerated timers (debug parameter)', () => {
  const s = new Session({ pingMs: 3000, warnAfterMs: 9000, answerMs: 6000 });
  const log = [];
  s.start(0);
  run(s, 0, 20_000, log);
  assert.deepEqual(log.filter(([a]) => a !== 'ping'), [['warn', 9000], ['disconnect', 15000]]);
});
