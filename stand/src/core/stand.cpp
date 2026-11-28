#include "core/stand.h"

#include <stdio.h>
#include <string.h>

#include "core/crc32.h"

namespace core {

namespace {

// Десятичное без знака из одних цифр, не больше max.
bool parse_uint(const char *s, uint32_t max, uint32_t &out) {
  if (!s || !*s) return false;
  uint64_t v = 0;
  for (const char *p = s; *p; p++) {
    if (*p < '0' || *p > '9') return false;
    v = v * 10 + (uint64_t)(*p - '0');
    if (v > max) return false;
  }
  out = (uint32_t)v;
  return true;
}

int hex_digit(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

bool parse_hex32(const char *s, uint32_t &out) {
  if (!s || strlen(s) != 8) return false;
  uint32_t v = 0;
  for (int i = 0; i < 8; i++) {
    int d = hex_digit(s[i]);
    if (d < 0) return false;
    v = (v << 4) | (uint32_t)d;
  }
  out = v;
  return true;
}

// Байты из hex-строки чётной длины; -1 — не hex или пусто.
int parse_hex_bytes(const char *s, uint8_t *out, size_t cap) {
  if (!s) return -1;
  size_t n = strlen(s);
  if (n == 0 || n % 2 || n / 2 > cap) return -1;
  for (size_t i = 0; i < n / 2; i++) {
    int hi = hex_digit(s[2 * i]), lo = hex_digit(s[2 * i + 1]);
    if (hi < 0 || lo < 0) return -1;
    out[i] = (uint8_t)(hi << 4 | lo);
  }
  return (int)(n / 2);
}

const char *arg(const char *s) { return s ? s : ""; }

bool is_lines_off(const char *line) { return strcmp(line, "#lines off") == 0; }

}  // namespace

bool Stand::step(uint32_t timeout_ms) {
  char line[Reader::MAX_LINE + 1];
  if (queued_ > 0) {
    memcpy(line, queue_[0], sizeof line);
    queued_--;
    memmove(queue_[0], queue_[1], sizeof queue_[0] * queued_);
  } else {
    Reader::Got got = d_.in.line(line, timeout_ms);
    if (got == Reader::Got::Timeout) return false;
    if (got == Reader::Got::TooLong) {
      d_.out.reply("слишком длинная команда");
      return true;
    }
  }
  if (line[0] == 0) return true;
  execute(line);
  return true;
}

void Stand::execute(char *line) {
  char *argv[4] = {nullptr, nullptr, nullptr, nullptr};
  int argc = 0;
  for (char *p = strtok(line, " "); p && argc < 4; p = strtok(nullptr, " ")) argv[argc++] = p;
  if (argc == 0) return;
  const char *name = argv[0][0] == '#' ? argv[0] + 1 : argv[0];

  if (!strcmp(name, "info")) return cmd_info();
  if (!strcmp(name, "heap")) return cmd_heap();
  if (!strcmp(name, "press")) return cmd_press(argv[1], argv[2]);
  if (!strcmp(name, "lines")) return cmd_lines(argv[1]);
  if (!strcmp(name, "power")) return cmd_power();
  if (!strcmp(name, "ir")) return cmd_ir(argv[1]);
  if (!strcmp(name, "chip")) return cmd_chip();
  if (!strcmp(name, "flash")) return cmd_flash(argv[1], argv[2]);
  if (!strcmp(name, "erase")) return cmd_erase(argv[1]);
  if (!strcmp(name, "ble")) return cmd_ble(argv[1], argv[2], argv[3]);
  d_.out.reply("неизвестная команда: %s", name);
}

void Stand::cmd_info() {
  d_.out.reply("ulpanir_stand %s uptime=%lu", d_.version, (unsigned long)(d_.clock.now_ms() / 1000));
}

// Задача 8.1: хватает ли кучи на образ 256 КБ (design §5).
void Stand::cmd_heap() {
  d_.out.reply("heap: max=%lu free=%lu", (unsigned long)d_.mem.largest(), (unsigned long)d_.mem.free_total());
}

// ---------------------------------------------------------------- кнопки

bool Stand::hold(uint32_t ms) {
  uint32_t start = d_.clock.now_ms();
  for (;;) {
    uint32_t spent = d_.clock.now_ms() - start;
    if (spent >= ms) return true;
    uint32_t left = ms - spent;
    // `#flash` в очереди: за ним идут байты образа, строками их не читать.
    bool flash_queued = queued_ > 0 && strncmp(queue_[queued_ - 1], "#flash", 6) == 0;
    if (queued_ == QUEUE || flash_queued) {
      d_.clock.sleep_ms(left);
      continue;
    }
    char line[Reader::MAX_LINE + 1];
    Reader::Got got = d_.in.line(line, left < 20 ? left : 20);
    if (got == Reader::Got::Line) {
      if (is_lines_off(line)) {
        d_.lines.release_all();
        return false;
      }
      if (line[0]) memcpy(queue_[queued_++], line, sizeof line);
    } else if (got == Reader::Got::TooLong) {
      d_.out.reply("слишком длинная команда");
    }
  }
}

void Stand::cmd_press(char *button, char *ms_s) {
  uint8_t mask = 0;
  const char *b = arg(button);
  if (!strcmp(b, "a"))
    mask = BTN_A;
  else if (!strcmp(b, "b"))
    mask = BTN_B;
  else if (!strcmp(b, "ab"))
    mask = BTN_A | BTN_B;
  else
    return d_.out.reply("press: неизвестная кнопка: %s", b);
  uint32_t ms;
  if (!parse_uint(ms_s, PRESS_MAX_MS, ms) || ms == 0)
    return d_.out.reply("press: неизвестная длительность: %s", arg(ms_s));

  d_.lines.press(mask);
  if (!hold(ms)) return d_.out.reply("lines: отпущены");
  d_.lines.release_buttons();
  d_.out.reply("press: %s %lu", b, (unsigned long)ms);
}

void Stand::cmd_lines(char *mode) {
  if (strcmp(arg(mode), "off")) return d_.out.reply("lines: неизвестный режим: %s", arg(mode));
  d_.lines.release_all();
  d_.out.reply("lines: отпущены");
}

// ---------------------------------------------------------------- питание и ИК

void Stand::cmd_power() {
  int mv = d_.adc.read_mv();
  if (mv < 0) mv = 0;
  int cv = (mv + 5) / 10;
  d_.out.reply("power: %s %d.%02d В", mv >= POWER_ON_MV ? "вкл" : "выкл", cv / 100, cv % 100);
}

void Stand::cmd_ir(char *mode) {
  const char *m = arg(mode);
  if (!strcmp(m, "on")) {
    d_.ir.on(d_.clock.now_ms());
    return d_.out.reply("ir: вкл");
  }
  if (!strcmp(m, "off")) {
    d_.ir.off();
    return d_.out.reply("ir: выкл");
  }
  d_.out.reply("ir: неизвестный режим: %s", m);
}

// ---------------------------------------------------------------- отладка CH592

const char *Stand::family_name(uint8_t family, char *buf, size_t cap) {
  switch (family) {
    case 0x01: return "CH32V10x";
    case 0x02: return "CH57x";
    case 0x05: return "CH32V20x";
    case 0x06: return "CH32V30x";
    case 0x07: return "CH58x";
    case 0x09: return "CH32V003";
    case FAMILY_CH59X: return "CH59x";
    case 0x4b: return "CH585";
    case 0x8b: return "CH570";
  }
  snprintf(buf, cap, "0x%02x", family);
  return buf;
}

bool Stand::session(ChipId &id) {
  // Узел A в 0: Q1 открыт и МК питается, даже когда его ядро остановлено (design §6).
  d_.lines.press(BTN_A);
  d_.clock.sleep_ms(SESSION_POWER_MS);
  if (d_.debug.attach(id)) return true;
  d_.debug.detach();
  d_.lines.release_buttons();
  return false;
}

void Stand::end_session() {
  d_.debug.reset();
  d_.lines.release_buttons();
}

void Stand::cmd_chip() {
  ChipId id;
  if (!session(id)) return d_.out.reply("chip: нет ответа");
  end_session();
  char buf[8];
  if (id.family != FAMILY_CH59X)
    return d_.out.reply("chip: не CH59x: %s", family_name(id.family, buf, sizeof buf));
  char uid[17];
  for (int i = 0; i < 8; i++) snprintf(uid + 2 * i, 3, "%02x", id.uid[i]);
  d_.out.reply("chip: CH59x uid=%s", uid);
}

void Stand::cmd_flash(char *len_s, char *crc_s) {
  uint32_t t0 = d_.clock.now_ms();
  uint32_t len, crc;
  if (!parse_uint(len_s, FLASH_MAX_LEN, len) || len == 0)
    return d_.out.reply("flash: неверная длина: %s", arg(len_s));
  if (!parse_hex32(crc_s, crc)) return d_.out.reply("flash: неверный crc32: %s", arg(crc_s));

  uint32_t padded = (len + FLASH_BLOCK - 1) / FLASH_BLOCK * FLASH_BLOCK;
  uint8_t *img = d_.mem.alloc(padded);
  if (!img) {
    // Байты образа всё равно придут — забрать, чтобы они не читались командами.
    uint8_t sink[256];
    for (uint32_t left = len; left;) {
      size_t k = d_.in.bytes(sink, left < sizeof sink ? left : sizeof sink, FLASH_IDLE_MS);
      if (k == 0) break;
      left -= (uint32_t)k;
    }
    return d_.out.reply("flash: нет памяти: %lu байт", (unsigned long)len);
  }

  const char *err = nullptr;
  char errbuf[48];
  if (d_.in.bytes(img, len, FLASH_IDLE_MS) < len) {
    err = "обрыв приёма";
  } else if (crc32(img, len) != crc) {
    err = "crc32 не сходится";
  } else {
    memset(img + len, 0xFF, padded - len);
    ChipId id;
    if (!session(id)) {
      err = "нет ответа чипа";
    } else {
      char buf[8];
      if (id.family != FAMILY_CH59X) {
        snprintf(errbuf, sizeof errbuf, "не CH59x: %s", family_name(id.family, buf, sizeof buf));
        err = errbuf;
      } else if (!d_.debug.erase(0, padded, Area::Program) || !d_.debug.write(0, img, padded)) {
        err = "нет ответа чипа";
      } else {
        for (uint32_t off = 0; off < padded; off += FLASH_BLOCK) {
          if (!d_.debug.verify(off, img + off, FLASH_BLOCK)) {
            snprintf(errbuf, sizeof errbuf, "сверка не сошлась 0x%05lx", (unsigned long)off);
            err = errbuf;
            break;
          }
        }
      }
      // Перезапуск и сразу отпустить A: пульт выключится сам (design §6).
      end_session();
    }
  }
  d_.mem.release(img);
  if (err) return d_.out.reply("flash: %s", err);
  d_.out.reply("flash: записано %lu байт за %lu мс", (unsigned long)len, (unsigned long)(d_.clock.now_ms() - t0));
}

void Stand::cmd_erase(char *area) {
  if (strcmp(arg(area), "data")) return d_.out.reply("erase: неизвестная область: %s", arg(area));
  ChipId id;
  if (!session(id)) return d_.out.reply("erase: нет ответа чипа");
  char buf[8];
  if (id.family != FAMILY_CH59X) {
    end_session();
    return d_.out.reply("erase: не CH59x: %s", family_name(id.family, buf, sizeof buf));
  }
  bool ok = d_.debug.erase(DATAFLASH_ADDR, DATAFLASH_SIZE, Area::DataFlash);
  end_session();
  if (!ok) return d_.out.reply("erase: нет ответа чипа");
  d_.out.reply("erase: настройки стёрты");
}

// ---------------------------------------------------------------- BLE

void Stand::cmd_ble(char *sub, char *a1, char *a2) {
  const char *s = arg(sub);
  if (!strcmp(s, "connect")) {
    uint32_t ms = BLE_DEFAULT_MS;
    if (a1 && (!parse_uint(a1, BLE_MAX_MS, ms) || ms == 0))
      return d_.out.reply("ble: неизвестная длительность: %s", a1);
    if (d_.ble.connected()) d_.ble.close();
    char name[32];
    int rssi = 0;
    if (!d_.ble.connect(ms, name, sizeof name, &rssi)) return d_.out.reply("ble: не найден");
    return d_.out.reply("ble: подключён %s rssi=%d", name, rssi);
  }
  if (!strcmp(s, "close")) {
    d_.ble.close();
    return d_.out.reply("ble: отключён");
  }
  if (!strcmp(s, "read")) {
    const char *c = arg(a1);
    Chr chr;
    if (!strcmp(c, "config"))
      chr = Chr::Config;
    else if (!strcmp(c, "status"))
      chr = Chr::Status;
    else
      return d_.out.reply("ble: неизвестная характеристика: %s", c);
    if (!d_.ble.connected()) return d_.out.reply("ble: нет подключения");
    uint8_t data[32];
    int n = d_.ble.read(chr, data, sizeof data);
    if (n < 0) return d_.out.reply("ble: нет подключения");
    char hex[2 * sizeof data + 1];
    hex[0] = 0;
    for (int i = 0; i < n; i++) snprintf(hex + 2 * i, 3, "%02x", data[i]);
    return d_.out.reply("ble: %s %s", c, hex);
  }
  if (!strcmp(s, "write")) {
    const char *c = arg(a1);
    Chr chr;
    if (!strcmp(c, "config"))
      chr = Chr::Config;
    else if (!strcmp(c, "ping"))
      chr = Chr::Ping;
    else if (!strcmp(c, "test"))
      chr = Chr::Test;
    else
      return d_.out.reply("ble: неизвестная характеристика: %s", c);
    uint8_t data[24];
    int n = parse_hex_bytes(a2, data, sizeof data);
    if (n < 0) return d_.out.reply("ble: неверные данные: %s", arg(a2));
    if (!d_.ble.connected()) return d_.out.reply("ble: нет подключения");
    switch (d_.ble.write(chr, data, (size_t)n)) {
      case Ble::Write::Ok: return d_.out.reply("ble: записано %s", c);
      case Ble::Write::Rejected: return d_.out.reply("ble: отклонено %s", c);
      default: return d_.out.reply("ble: нет подключения");
    }
  }
  d_.out.reply("ble: неизвестная команда: %s", s);
}

}  // namespace core
