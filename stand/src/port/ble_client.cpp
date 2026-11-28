#include "port/ble_client.h"

#include <BLEDevice.h>
#include <string.h>

namespace {

// docs/ble_protocol.md, раздел «UUID»
const char *SERVICE_UUID = "d5e80001-b2f6-4b31-98c6-7923e11860fc";
const char *CHR_UUID[4] = {
    "d5e80002-b2f6-4b31-98c6-7923e11860fc",  // config
    "d5e80003-b2f6-4b31-98c6-7923e11860fc",  // status
    "d5e80004-b2f6-4b31-98c6-7923e11860fc",  // ping
    "d5e80005-b2f6-4b31-98c6-7923e11860fc",  // test
};
const char *NAME_PREFIX = "IR-Remote-";

}  // namespace

// Гипотеза: BLE central ядра работает на C6 (design §9); запасной вариант — библиотека NimBLE-Arduino.
bool BleClient::connect(uint32_t timeout_ms, char *name, size_t name_cap, int *rssi) {
  close();
  if (!inited_) {
    BLEDevice::init("");
    inited_ = true;
  }
  BLEScan *scan = BLEDevice::getScan();
  scan->setActiveScan(true);  // имя пульта может прийти в ответе на сканирование
  scan->clearResults();
  BLEScanResults *res = scan->start((timeout_ms + 999) / 1000, false);
  if (!res) return false;

  BLEUUID service(SERVICE_UUID);
  int best = -1, best_rssi = -1000;
  for (int i = 0; i < res->getCount(); i++) {
    BLEAdvertisedDevice d = res->getDevice(i);
    if (!d.isAdvertisingService(service)) continue;
    if (d.haveName() && !d.getName().startsWith(NAME_PREFIX)) continue;
    if (d.getRSSI() > best_rssi) {
      best = i;
      best_rssi = d.getRSSI();
    }
  }
  if (best < 0) return false;

  BLEAdvertisedDevice dev = res->getDevice(best);
  if (!client_) client_ = BLEDevice::createClient();
  if (!client_->connect(&dev)) return false;
  BLERemoteService *svc = client_->getService(service);
  bool ok = svc != nullptr;
  for (int i = 0; ok && i < 4; i++) {
    chr_[i] = svc->getCharacteristic(BLEUUID(CHR_UUID[i]));
    ok = chr_[i] != nullptr;
  }
  if (!ok) {
    close();
    return false;
  }
  snprintf(name, name_cap, "%s", dev.haveName() ? dev.getName().c_str() : NAME_PREFIX);
  *rssi = dev.getRSSI();
  return true;
}

bool BleClient::connected() { return client_ && client_->isConnected() && chr_[0]; }

int BleClient::read(core::Chr chr, uint8_t *out, size_t max) {
  if (!connected()) return -1;
  String v = chr_[(int)chr]->readValue();
  if (!client_->isConnected()) return -1;
  size_t n = v.length() < max ? v.length() : max;
  memcpy(out, v.c_str(), n);
  return (int)n;
}

// Отказ пульта — ATT-ошибка 0x80 (docs/ble_protocol.md «Ошибки»): запись не прошла, а связь осталась.
core::Ble::Write BleClient::write(core::Chr chr, const uint8_t *data, size_t len) {
  if (!connected()) return Write::NotConnected;
  if (chr_[(int)chr]->writeValue((uint8_t *)data, len, true)) return Write::Ok;
  return client_->isConnected() ? Write::Rejected : Write::NotConnected;
}

void BleClient::close() {
  if (client_ && client_->isConnected()) client_->disconnect();
  for (auto &c : chr_) c = nullptr;
}
