// core::Ble на BLE-клиенте ядра Arduino-ESP32 3.x (на C6 — стек NimBLE), UUID — docs/ble_protocol.md.
#pragma once
#include "core/ble.h"

class BLEClient;
class BLERemoteCharacteristic;

class BleClient final : public core::Ble {
 public:
  bool connect(uint32_t timeout_ms, char *name, size_t name_cap, int *rssi) override;
  bool connected() override;
  int read(core::Chr chr, uint8_t *out, size_t max) override;
  Write write(core::Chr chr, const uint8_t *data, size_t len) override;
  void close() override;

 private:
  bool inited_ = false;
  BLEClient *client_ = nullptr;
  BLERemoteCharacteristic *chr_[4] = {};
};
