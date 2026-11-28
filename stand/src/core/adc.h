// Напряжение VBAT_SW (J1.5) после ключа Q1 и порог «включён» (design §6).
#pragma once
#include <stdint.h>

namespace core {

// Гипотеза: ниже 1,5 В МК уже не работает; подобрать на плате по спаду VBAT_SW (задача 8.3).
constexpr int POWER_ON_MV = 1500;

class Adc {
 public:
  virtual int read_mv() = 0;

 protected:
  ~Adc() = default;
};

}  // namespace core
