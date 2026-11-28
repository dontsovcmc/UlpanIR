// Спека stand-flash: `#chip`, `#flash`, `#erase data` на поддельной отладке.
#include <unity.h>

#include "../fakes/fakes.h"
#include "core/crc32.h"

static Rig *r;
void setUp() { r = new Rig; }
void tearDown() { delete r; }

static std::vector<uint8_t> image(size_t n) {
  std::vector<uint8_t> v(n);
  for (size_t i = 0; i < n; i++) v[i] = (uint8_t)(i * 7 + 3);
  return v;
}

static std::string header(size_t n, uint32_t crc) {
  char buf[64];
  snprintf(buf, sizeof buf, "#flash %zu %08x\n", n, crc);
  return buf;
}

// Порядок «A в 0 → сеанс → перезапуск → A отпущен».
static void assert_session_order() {
  int p = r->j.find("press 1");
  int a = r->j.find("attach", p);
  int rs = r->j.find("reset", a);
  int rel = r->j.find("release", rs);
  TEST_ASSERT_TRUE(p >= 0);
  TEST_ASSERT_TRUE(a > p);
  TEST_ASSERT_TRUE(rs > a);
  TEST_ASSERT_TRUE(rel > rs);
  TEST_ASSERT_EQUAL(0, r->lines.low);
}

void test_crc32_like_zlib() {
  TEST_ASSERT_EQUAL_HEX32(0xCBF43926, core::crc32((const uint8_t *)"123456789", 9));
  TEST_ASSERT_EQUAL_HEX32(0, core::crc32(nullptr, 0));
}

// «Образ записан»
void test_image_written() {
  auto img = image(4096);
  r->io.send(header(img.size(), core::crc32(img.data(), img.size())));
  r->io.send_bytes(img);
  r->run();
  TEST_ASSERT_TRUE(r->j.find("erase 0 4096 program") >= 0);
  TEST_ASSERT_TRUE(r->j.find("write 0 4096") >= 0);
  TEST_ASSERT_EQUAL_UINT32(4096, r->debug.verified_len);
  TEST_ASSERT_EQUAL_MEMORY(img.data(), r->debug.chip, 4096);
  assert_session_order();
  TEST_ASSERT_TRUE(r->out.all.rfind("# flash: записано 4096 байт за ", 0) == 0);
}

// Образ одним куском с командой и длина не кратна 256: хвост сектора — 0xFF.
void test_image_tail_padded() {
  auto img = image(1000);
  std::string all = header(img.size(), core::crc32(img.data(), img.size())) + std::string(img.begin(), img.end());
  r->io.send(all);
  r->run();
  TEST_ASSERT_TRUE(r->j.find("erase 0 1024 program") >= 0);
  TEST_ASSERT_EQUAL_MEMORY(img.data(), r->debug.chip, 1000);
  TEST_ASSERT_EQUAL_HEX8(0xFF, r->debug.chip[1023]);
  TEST_ASSERT_TRUE(r->out.all.rfind("# flash: записано 1000 байт за ", 0) == 0);
}

// «Образ испорчен по дороге»
void test_image_corrupted() {
  auto img = image(1024);
  r->io.send(header(img.size(), core::crc32(img.data(), img.size()) ^ 1));
  r->io.send_bytes(img);
  r->run();
  TEST_ASSERT_EQUAL_STRING("# flash: crc32 не сходится\n", r->out.all.c_str());
  TEST_ASSERT_FALSE(r->debug.touched);
  TEST_ASSERT_EQUAL(-1, r->j.find("press"));
}

// «Обрыв приёма»
void test_receive_broken() {
  auto img = image(100);
  r->io.send(header(4096, 0x12345678));
  r->io.send_bytes(img);
  r->stand.step(0);
  TEST_ASSERT_EQUAL_STRING("# flash: обрыв приёма\n", r->out.all.c_str());
  TEST_ASSERT_GREATER_OR_EQUAL_UINT32(2000, r->out.time_of("обрыв"));
  TEST_ASSERT_LESS_THAN_UINT32(2200, r->out.time_of("обрыв"));
  TEST_ASSERT_FALSE(r->debug.touched);
}

void test_bad_length_and_crc() {
  r->io.send("#flash 0 00000000\n#flash 262145 00000000\n#flash 12a 00000000\n#flash 10 1234567\n#flash 10 1234567g\n");
  r->run();
  TEST_ASSERT_EQUAL_STRING(
      "# flash: неверная длина: 0\n# flash: неверная длина: 262145\n# flash: неверная длина: 12a\n"
      "# flash: неверный crc32: 1234567\n# flash: неверный crc32: 1234567g\n",
      r->out.all.c_str());
  TEST_ASSERT_FALSE(r->debug.touched);
}

void test_max_length_accepted() {
  auto img = image(262144);
  r->io.send(header(img.size(), core::crc32(img.data(), img.size())));
  r->io.send_bytes(img);
  r->run();
  TEST_ASSERT_TRUE(r->out.all.rfind("# flash: записано 262144 байт за ", 0) == 0);
}

void test_verify_mismatch() {
  auto img = image(4096);
  r->debug.corrupt_at = 0x512;
  r->io.send(header(img.size(), core::crc32(img.data(), img.size())));
  r->io.send_bytes(img);
  r->run();
  TEST_ASSERT_EQUAL_STRING("# flash: сверка не сошлась 0x00500\n", r->out.all.c_str());
  assert_session_order();
}

void test_flash_no_chip() {
  auto img = image(256);
  r->debug.present = false;
  r->io.send(header(img.size(), core::crc32(img.data(), img.size())));
  r->io.send_bytes(img);
  r->run();
  TEST_ASSERT_EQUAL_STRING("# flash: нет ответа чипа\n", r->out.all.c_str());
  TEST_ASSERT_TRUE(r->j.find("detach") >= 0);
  TEST_ASSERT_EQUAL(0, r->lines.low);
}

void test_flash_wrong_family() {
  auto img = image(256);
  r->debug.family = 0x09;
  r->io.send(header(img.size(), core::crc32(img.data(), img.size())));
  r->io.send_bytes(img);
  r->run();
  TEST_ASSERT_EQUAL_STRING("# flash: не CH59x: CH32V003\n", r->out.all.c_str());
  TEST_ASSERT_EQUAL(-1, r->j.find("erase"));
  TEST_ASSERT_EQUAL(-1, r->j.find("write"));
  TEST_ASSERT_EQUAL(0, r->lines.low);
}

void test_flash_no_memory() {
  auto img = image(512);
  r->mem.fail = true;
  r->io.send(header(img.size(), core::crc32(img.data(), img.size())));
  r->io.send_bytes(img);
  r->io.send("#info\n");
  r->run();
  TEST_ASSERT_EQUAL_STRING("# flash: нет памяти: 512 байт\n# ulpanir_stand 0.1.0 uptime=0\n", r->out.all.c_str());
  TEST_ASSERT_FALSE(r->debug.touched);
}

// «Плата на месте»
void test_chip() {
  r->io.send("#chip\n");
  r->run();
  TEST_ASSERT_EQUAL_STRING("# chip: CH59x uid=123456789abcdef0\n", r->out.all.c_str());
  assert_session_order();
}

// «Платы нет»
void test_chip_absent() {
  r->debug.present = false;
  r->io.send("#chip\n");
  r->stand.step(0);
  TEST_ASSERT_EQUAL_STRING("# chip: нет ответа\n", r->out.all.c_str());
  TEST_ASSERT_LESS_OR_EQUAL_UINT32(2000, r->out.time_of("нет ответа"));
  TEST_ASSERT_EQUAL(0, r->lines.low);
}

void test_chip_wrong_family() {
  r->debug.family = 0x42;
  r->io.send("#chip\n");
  r->run();
  TEST_ASSERT_EQUAL_STRING("# chip: не CH59x: 0x42\n", r->out.all.c_str());
}

// «Пульт снова чистый» — часть стенда: стирается вся DataFlash, основная flash не тронута.
void test_erase_data() {
  memset(r->debug.chip, 0x5A, 0x1000);
  r->io.send("#erase data\n");
  r->run();
  TEST_ASSERT_EQUAL_STRING("# erase: настройки стёрты\n", r->out.all.c_str());
  TEST_ASSERT_TRUE(r->j.find("erase 458752 32768 data") >= 0);
  TEST_ASSERT_EQUAL_HEX8(0x5A, r->debug.chip[0]);
  assert_session_order();
}

void test_erase_unknown_area() {
  r->io.send("#erase all\n");
  r->run();
  TEST_ASSERT_EQUAL_STRING("# erase: неизвестная область: all\n", r->out.all.c_str());
  TEST_ASSERT_FALSE(r->debug.touched);
}

void test_erase_no_chip() {
  r->debug.present = false;
  r->io.send("#erase data\n");
  r->run();
  TEST_ASSERT_EQUAL_STRING("# erase: нет ответа чипа\n", r->out.all.c_str());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_crc32_like_zlib);
  RUN_TEST(test_image_written);
  RUN_TEST(test_image_tail_padded);
  RUN_TEST(test_image_corrupted);
  RUN_TEST(test_receive_broken);
  RUN_TEST(test_bad_length_and_crc);
  RUN_TEST(test_max_length_accepted);
  RUN_TEST(test_verify_mismatch);
  RUN_TEST(test_flash_no_chip);
  RUN_TEST(test_flash_wrong_family);
  RUN_TEST(test_flash_no_memory);
  RUN_TEST(test_chip);
  RUN_TEST(test_chip_absent);
  RUN_TEST(test_chip_wrong_family);
  RUN_TEST(test_erase_data);
  RUN_TEST(test_erase_unknown_area);
  RUN_TEST(test_erase_no_chip);
  return UNITY_END();
}
