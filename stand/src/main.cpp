// Точка сборки стенда: запрет сброса от USB, линии J1, две задачи — «команды» (loop) и «поток» (лог
// пульта и ИК-события в порт), design §2, §3.
#include <Arduino.h>
#include <soc/soc.h>
#include <soc/usb_serial_jtag_reg.h>

#include "core/console.h"
#include "core/reader.h"
#include "core/stand.h"
#include "port/adc_vbat.h"
#include "port/ble_client.h"
#include "port/debug_rvswd.h"
#include "port/host_usb.h"
#include "port/ir_rmt.h"
#include "port/lines_gpio.h"
#include "port/uart_log.h"

namespace {

void usb_write(void *, const uint8_t *data, size_t len) { Serial.write(data, len); }

MutexLock usb_lock;
core::Console console(usb_write, nullptr, &usb_lock);
UsbInput usb_in;
core::Reader reader(usb_in);
ClockEsp clock_esp;
LinesGpio lines;
AdcVbat adc;
DebugRvswd debug;
BleClient ble;
core::IrGate ir_gate;
HeapMemory heap;
UartLog uart_log;
IrRmt ir;

core::Stand stand(core::Deps{console, reader, clock_esp, lines, adc, debug, ble, ir_gate, heap, FW_VERSION});

void stream_task(void *) {
  uint8_t buf[256];
  for (;;) {
    size_t n = uart_log.read(buf, sizeof buf);
    if (n) console.log(buf, n);
    ir.poll(ir_gate, console);
    if (!n) vTaskDelay(1);
  }
}

}  // namespace

void setup() {
  // DTR/RTS с компьютера иначе перезагружают C6 посреди работы (опыт waterius_flasher)
  SET_PERI_REG_MASK(USB_SERIAL_JTAG_CHIP_RST_REG, USB_SERIAL_JTAG_USB_UART_CHIP_RST_DIS);
  lines.begin();
  debug.begin();
  adc.begin();
  usb_lock.begin();
  Serial.setRxBufferSize(16 * 1024);  // образ `#flash` идёт сплошным потоком
  Serial.begin();
  uart_log.begin();
  ir.begin();
  xTaskCreate(stream_task, "stream", 4096, nullptr, 2, nullptr);
}

void loop() { stand.step(1000); }
