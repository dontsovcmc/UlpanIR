// Что core/ берёт у железа: часы, байты из порта, мьютекс. Реализации — в port/, поддельные — в test/fakes/.
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace core {

class Clock {
 public:
  virtual uint32_t now_ms() = 0;
  virtual void sleep_ms(uint32_t ms) = 0;

 protected:
  ~Clock() = default;
};

// Байты команд из USB. 0 — за timeout_ms ничего не пришло.
class Input {
 public:
  virtual size_t read(uint8_t *out, size_t max, uint32_t timeout_ms) = 0;

 protected:
  ~Input() = default;
};

class Lock {
 public:
  virtual void lock() = 0;
  virtual void unlock() = 0;

 protected:
  ~Lock() = default;
};

// Буфер под образ `#flash`: на C6 — куча, в тестах — вектор.
class Memory {
 public:
  virtual uint8_t *alloc(size_t len) = 0;
  virtual void release(uint8_t *p) = 0;
  // Самый большой кусок, который alloc сейчас выдаст, и вся свободная память — для `#heap`.
  virtual size_t largest() = 0;
  virtual size_t free_total() = 0;

 protected:
  ~Memory() = default;
};

}  // namespace core
