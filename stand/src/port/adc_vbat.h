// core::Adc: VBAT_SW (J1.5) на GPIO0, ослабление 12 дБ, встроенная подтяжка к земле (design §6).
#pragma once
#include "core/adc.h"

class AdcVbat final : public core::Adc {
 public:
  void begin();
  int read_mv() override;
};
