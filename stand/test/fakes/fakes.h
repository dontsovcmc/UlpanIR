// Поддельное железо стенда для тестов core/ на Mac: часы, вход USB, линии, АЦП, отладка, BLE, память.
// Всё, что «происходит с железом», пишется в общий журнал с поддельным временем — по нему тесты проверяют
// порядок (A в 0 → сеанс → перезапуск → A отпущен) и длительности.
#pragma once
#include <stdio.h>
#include <string.h>

#include <deque>
#include <string>
#include <vector>

#include "core/adc.h"
#include "core/ble.h"
#include "core/console.h"
#include "core/debug.h"
#include "core/io.h"
#include "core/lines.h"
#include "core/reader.h"
#include "core/stand.h"

struct Journal {
  uint32_t now = 0;
  std::vector<std::string> events;  // "t=<мс> <что>"

  void add(const std::string &what) { events.push_back("t=" + std::to_string(now) + " " + what); }
  // Индекс первого события, начинающегося (после времени) с prefix; -1 — нет.
  int find(const std::string &prefix, int from = 0) const {
    for (int i = from; i < (int)events.size(); i++) {
      const std::string &e = events[i];
      size_t sp = e.find(' ');
      if (e.compare(sp + 1, prefix.size(), prefix) == 0) return i;
    }
    return -1;
  }
  uint32_t time_of(int i) const { return (uint32_t)atol(events[i].c_str() + 2); }
};

// Часы и вход USB: куски байт, каждый — после своей паузы. Нет данных — время идёт на весь timeout.
class FakeIo : public core::Clock, public core::Input {
 public:
  explicit FakeIo(Journal &j) : j_(j) {}

  void send(const std::string &data, uint32_t gap_ms = 0) { chunks_.push_back({gap_ms, data}); }
  void send_bytes(const std::vector<uint8_t> &data, uint32_t gap_ms = 0) {
    send(std::string(data.begin(), data.end()), gap_ms);
  }

  uint32_t now_ms() override { return j_.now; }
  void sleep_ms(uint32_t ms) override { j_.now += ms; }

  size_t read(uint8_t *out, size_t max, uint32_t timeout_ms) override {
    if (chunks_.empty()) {
      j_.now += timeout_ms;
      return 0;
    }
    Chunk &c = chunks_.front();
    if (c.gap_ms > timeout_ms) {
      c.gap_ms -= timeout_ms;
      j_.now += timeout_ms;
      return 0;
    }
    j_.now += c.gap_ms;
    c.gap_ms = 0;
    size_t n = c.data.size() < max ? c.data.size() : max;
    memcpy(out, c.data.data(), n);
    c.data.erase(0, n);
    if (c.data.empty()) chunks_.pop_front();
    return n;
  }

  bool idle() const { return chunks_.empty(); }

 private:
  struct Chunk {
    uint32_t gap_ms;
    std::string data;
  };
  Journal &j_;
  std::deque<Chunk> chunks_;
};

// Выход USB: всё подряд и каждая запись с временем.
struct FakeOut {
  Journal &j;
  std::string all;
  std::vector<std::pair<uint32_t, std::string>> writes;

  explicit FakeOut(Journal &jj) : j(jj) {}
  static void sink(void *ctx, const uint8_t *data, size_t len) {
    auto *o = (FakeOut *)ctx;
    std::string s((const char *)data, len);
    o->all += s;
    o->writes.push_back({o->j.now, s});
  }
  // Время записи, содержащей text; UINT32_MAX — не было.
  uint32_t time_of(const std::string &text) const {
    for (auto &w : writes)
      if (w.second.find(text) != std::string::npos) return w.first;
    return UINT32_MAX;
  }
};

class FakeLines : public core::Lines {
 public:
  explicit FakeLines(Journal &j) : j_(j) {}
  uint8_t low = 0;  // узлы, выведенные в 0
  bool drove_high = false;  // по интерфейсу «0 или вход» в 1 вывести нельзя — флаг для наглядности

  void press(uint8_t mask) override {
    low |= mask;
    j_.add("press " + std::to_string(mask));
  }
  void release_buttons() override {
    low = 0;
    j_.add("release");
  }
  void release_all() override {
    low = 0;
    j_.add("release_all");
  }

 private:
  Journal &j_;
};

class FakeAdc : public core::Adc {
 public:
  std::deque<int> mv;
  int read_mv() override {
    if (mv.empty()) return 0;
    int v = mv.front();
    if (mv.size() > 1) mv.pop_front();
    return v;
  }
};

class FakeDebug : public core::Debug {
 public:
  explicit FakeDebug(Journal &j) : j_(j) { memset(chip, 0xFF, sizeof chip); }

  bool present = true;
  uint8_t family = core::FAMILY_CH59X;
  uint8_t uid[8] = {0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0};
  int64_t corrupt_at = -1;  // этот байт записывается неверно
  uint8_t chip[512 * 1024];
  uint32_t erased_from = 0, erased_len = 0, written_len = 0, verified_len = 0;
  bool touched = false;  // к отладке обращались

  bool attach(core::ChipId &id) override {
    touched = true;
    j_.add("attach");
    if (!present) {
      j_.now += 300;  // funprog перебирает протоколы
      return false;
    }
    id.family = family;
    memcpy(id.uid, uid, 8);
    return true;
  }
  bool erase(uint32_t addr, uint32_t len, core::Area area) override {
    j_.add("erase " + std::to_string(addr) + " " + std::to_string(len) +
           (area == core::Area::DataFlash ? " data" : " program"));
    memset(chip + addr, 0xFF, len);
    erased_from = addr;
    erased_len = len;
    return true;
  }
  bool write(uint32_t addr, const uint8_t *data, uint32_t len) override {
    j_.add("write " + std::to_string(addr) + " " + std::to_string(len));
    memcpy(chip + addr, data, len);
    if (corrupt_at >= addr && corrupt_at < addr + len) chip[corrupt_at] ^= 0x01;
    written_len += len;
    j_.now += len / 1024;  // около 1 мс на КБ
    return true;
  }
  bool verify(uint32_t addr, const uint8_t *data, uint32_t len) override {
    if (verified_len == 0) j_.add("verify");
    verified_len += len;
    return memcmp(chip + addr, data, len) == 0;
  }
  void reset() override { j_.add("reset"); }
  void detach() override { j_.add("detach"); }

 private:
  Journal &j_;
};

class FakeBle : public core::Ble {
 public:
  explicit FakeBle(Journal &j) : j_(j) {}

  bool found = true;
  std::string name = "IR-Remote-1A2B";
  int rssi = -61;
  bool is_connected = false;
  std::vector<uint8_t> config = {1, 1, 0x32, 0, 2, 1, 0x32, 0, 3, 3, 0};
  std::vector<uint8_t> status = {1, 0, 1, 0, 0xe4, 0x0c};
  bool reject = false;
  std::string last_write;

  bool connect(uint32_t timeout_ms, char *out, size_t cap, int *r) override {
    if (!found) {
      j_.now += timeout_ms;
      return false;
    }
    is_connected = true;
    snprintf(out, cap, "%s", name.c_str());
    *r = rssi;
    return true;
  }
  bool connected() override { return is_connected; }
  int read(core::Chr chr, uint8_t *out, size_t max) override {
    if (!is_connected) return -1;
    auto &v = chr == core::Chr::Config ? config : status;
    size_t n = v.size() < max ? v.size() : max;
    memcpy(out, v.data(), n);
    return (int)n;
  }
  Write write(core::Chr chr, const uint8_t *data, size_t len) override {
    if (!is_connected) return Write::NotConnected;
    if (reject) return Write::Rejected;
    last_write = std::to_string((int)chr) + ":" + std::string((const char *)data, len);
    if (chr == core::Chr::Config) config.assign(data, data + len);
    return Write::Ok;
  }
  void close() override { is_connected = false; }

 private:
  Journal &j_;
};

class FakeMem : public core::Memory {
 public:
  bool fail = false;
  uint8_t *alloc(size_t len) override { return fail ? nullptr : new uint8_t[len]; }
  void release(uint8_t *p) override { delete[] p; }
  size_t largest() override { return 324 * 1024; }
  size_t free_total() override { return 392 * 1024; }
};

// Стенд целиком на поддельном железе.
struct Rig {
  Journal j;
  FakeIo io{j};
  FakeOut out{j};
  core::Console console{FakeOut::sink, &out};
  core::Reader reader{io};
  FakeLines lines{j};
  FakeAdc adc;
  FakeDebug debug{j};
  FakeBle ble{j};
  core::IrGate ir;
  FakeMem mem;
  core::Stand stand{core::Deps{console, reader, io, lines, adc, debug, ble, ir, mem, "0.1.0"}};

  // Выполнять команды, пока вход не опустел, и ещё одну паузу.
  void run() {
    for (int guard = 0; guard < 100; guard++) {
      bool any = stand.step(100);
      if (!any && io.idle()) break;
    }
  }
};
