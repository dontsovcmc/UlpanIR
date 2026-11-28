// Команды стенда (specs stand-port, stand-flash, stand-test): строка `#…` → действие → ответ `# …`.
// Выполняются по одной в задаче «команды»; блокироваться им можно (design §3).
#pragma once
#include <atomic>
#include <stddef.h>
#include <stdint.h>

#include "core/adc.h"
#include "core/ble.h"
#include "core/console.h"
#include "core/debug.h"
#include "core/io.h"
#include "core/lines.h"
#include "core/reader.h"

namespace core {

constexpr uint32_t PRESS_MAX_MS = 10000;
constexpr uint32_t FLASH_MAX_LEN = 256 * 1024;
constexpr uint32_t FLASH_IDLE_MS = 2000;      // обрыв приёма образа
constexpr uint32_t BLE_DEFAULT_MS = 10000;
constexpr uint32_t BLE_MAX_MS = 60000;
// Гипотеза: C4 22 мкФ заряжается через Q1 и МК стартует быстрее 50 мс после нажатия A.
constexpr uint32_t SESSION_POWER_MS = 50;

// Приём ИК включён командой `#ir on`; события выводит задача «поток» (design §8).
class IrGate {
 public:
  void on(uint32_t now_ms) {
    t0_ = now_ms;
    on_ = true;
  }
  void off() { on_ = false; }
  bool enabled() const { return on_; }
  uint32_t since(uint32_t now_ms) const { return now_ms - t0_; }

 private:
  std::atomic<bool> on_{false};
  std::atomic<uint32_t> t0_{0};
};

struct Deps {
  Console &out;
  Reader &in;
  Clock &clock;
  Lines &lines;
  Adc &adc;
  Debug &debug;
  Ble &ble;
  IrGate &ir;
  Memory &mem;
  const char *version;
};

class Stand {
 public:
  explicit Stand(const Deps &d) : d_(d) {}

  // Одна команда — из очереди или из порта. false — за timeout_ms команды не было.
  bool step(uint32_t timeout_ms);

 private:
  static constexpr int QUEUE = 4;

  void execute(char *line);
  void cmd_info();
  void cmd_heap();
  void cmd_press(char *button, char *ms);
  void cmd_lines(char *mode);
  void cmd_power();
  void cmd_ir(char *mode);
  void cmd_chip();
  void cmd_flash(char *len, char *crc);
  void cmd_erase(char *area);
  void cmd_ble(char *sub, char *a1, char *a2);

  // Держать узлы mask ms миллисекунд; строки, пришедшие за это время, — в очередь. false — пришёл
  // `#lines off`, линии отпущены.
  bool hold(uint32_t ms);
  bool session(ChipId &id);  // A в 0 → подключение; при отказе всё отпущено
  void end_session();        // перезапуск → A отпущен
  const char *family_name(uint8_t family, char *buf, size_t cap);

  Deps d_;
  char queue_[QUEUE][Reader::MAX_LINE + 1];
  int queued_ = 0;
};

}  // namespace core
