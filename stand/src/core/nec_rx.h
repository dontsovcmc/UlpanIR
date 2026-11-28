// Разбор ИК-кадра по длительностям с приёмника (design §8): метка, пауза, метка… начиная с метки, в мкс.
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace core {

struct IrFrame {
  enum Kind : uint8_t { Nec, Repeat, Unparsed } kind;
  bool addr16;       // второй байт — не инверсия первого: 16-битный адрес
  uint16_t address;
  uint8_t command;
  uint16_t pulses;   // меток в кадре
};

// Допуск ±25 %: приёмник искажает ширину импульсов.
IrFrame nec_decode(const uint32_t *dur_us, size_t n);

// Склеить помехи короче min_us с соседями (аппаратный фильтр RMT короче 100 мкс не бывает). Возвращает
// новое число длительностей.
size_t ir_filter(uint32_t *dur_us, size_t n, uint32_t min_us);

// Строка события без `# `: `ir: nec 0x32 0x02 t=…`, `ir: повтор t=…`, `ir: неразобрано 5 t=…`.
void ir_format(const IrFrame &f, uint32_t t_ms, char *out, size_t cap);

}  // namespace core
