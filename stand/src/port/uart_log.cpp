#include "port/uart_log.h"

#include <Arduino.h>
#include <driver/gpio.h>

#include "pins.h"

// Ядро по умолчанию подтягивает RX к 3,3 В (_uartApplyRxPull, опыт waterius_flasher): через PA9 это
// подпитывало бы выключенный пульт. TX = -1: ядро ногу TX не подключает.
void UartLog::begin() {
  Serial1.setRxBufferSize(8 * 1024);
  Serial1.enableRxInternalPull(false);
  Serial1.begin(115200, SERIAL_8N1, PIN_LOG_RX, -1);
  gpio_pullup_dis((gpio_num_t)PIN_LOG_RX);
}

size_t UartLog::read(uint8_t *out, size_t max) {
  int n = Serial1.available();
  if (n <= 0) return 0;
  return Serial1.read(out, (size_t)n < max ? (size_t)n : max);
}
