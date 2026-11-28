// CRC-32 как zlib.crc32 (полином 0xEDB88320, начальное и конечное 0xFFFFFFFF).
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace core {

uint32_t crc32(const uint8_t *data, size_t len);

}  // namespace core
