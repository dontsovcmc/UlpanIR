// BLE-клиент к пульту в режиме настройки (docs/ble_protocol.md). Реализация — port/ble_client.
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace core {

enum class Chr : uint8_t { Config, Status, Ping, Test };

class Ble {
 public:
  enum class Write : uint8_t { Ok, Rejected, NotConnected };

  // Найти `IR-Remote-…` с сервисом настройки и подключиться. false — не найден или не подключился.
  virtual bool connect(uint32_t timeout_ms, char *name, size_t name_cap, int *rssi) = 0;
  virtual bool connected() = 0;
  // Длина прочитанного; -1 — нет подключения.
  virtual int read(Chr chr, uint8_t *out, size_t max) = 0;
  virtual Write write(Chr chr, const uint8_t *data, size_t len) = 0;
  virtual void close() = 0;

 protected:
  ~Ble() = default;
};

}  // namespace core
