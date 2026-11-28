#include "port/debug_rvswd.h"

#include "port/funprog_glue.h"

void DebugRvswd::begin() { fp_detach(); }

bool DebugRvswd::attach(core::ChipId &id) { return fp_attach(&id.family, id.uid) == 0; }

bool DebugRvswd::erase(uint32_t addr, uint32_t len, core::Area area) {
  return fp_erase(addr, len, area == core::Area::DataFlash) == 0;
}

bool DebugRvswd::write(uint32_t addr, const uint8_t *data, uint32_t len) { return fp_write(addr, data, len) == 0; }

bool DebugRvswd::verify(uint32_t addr, const uint8_t *data, uint32_t len) {
  return fp_verify(addr, data, len) == 0;
}

void DebugRvswd::reset() { fp_reset(); }

void DebugRvswd::detach() { fp_detach(); }
