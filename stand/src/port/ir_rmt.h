// Приём ИК: RMT RX на выходе приёмника 38 кГц, шаг 1 мкс, конец кадра — тишина 12 мс (design §8).
#pragma once
#include "core/console.h"
#include "core/stand.h"

class IrRmt {
 public:
  bool begin();
  // Из задачи «поток»: кадр принят — разобрать и, если `#ir on`, вывести строку.
  void poll(core::IrGate &gate, core::Console &out);

 private:
  void start();
  bool ok_ = false;
};
