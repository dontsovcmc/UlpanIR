// Спека stand-test «Приём ИК командой #ir», «Повтор и неразобранные кадры»: круг «кодировщик пульта
// firmware/src/nec.c → искажение → разбор стенда».
#include <unity.h>

#include "../fakes/fakes.h"
#include "core/nec_rx.h"

extern "C" {
#include "nec.h"
}
#include "nec.c"

void setUp() {}
void tearDown() {}

// Длительности кадра пульта в мкс, каждая искажена на +pct % или −pct % по очереди.
static size_t remote_frame(uint16_t addr, uint8_t cmd, bool ext, int pct, uint32_t *out) {
  nec_frame_t f;
  nec_encode(addr, cmd, ext, &f);
  for (int i = 0; i < f.count; i++) {
    double us = f.half_us[i] / 2.0;
    out[i] = (uint32_t)(us * (100 + (i % 2 ? -pct : pct)) / 100.0);
  }
  return f.count;
}

static std::string line(const uint32_t *d, size_t n, uint32_t t) {
  char buf[64];
  core::ir_format(core::nec_decode(d, n), t, buf, sizeof buf);
  return buf;
}

// «Кадр кнопки A»
void test_frame_button_a() {
  uint32_t d[80];
  size_t n = remote_frame(0x32, 0x02, false, 20, d);
  TEST_ASSERT_EQUAL_STRING("ir: nec 0x32 0x02 t=1234", line(d, n, 1234).c_str());
}

void test_frame_shrunk() {
  uint32_t d[80];
  size_t n = remote_frame(0x32, 0x03, false, -20, d);
  TEST_ASSERT_EQUAL_STRING("ir: nec 0x32 0x03 t=0", line(d, n, 0).c_str());
}

void test_frame_16bit_address() {
  uint32_t d[80];
  size_t n = remote_frame(0x1234, 0xa5, true, 10, d);
  TEST_ASSERT_EQUAL_STRING("ir: nec 0x1234 0xa5 t=7", line(d, n, 7).c_str());
}

// Пауза после стоп-метки, если RMT её отдал, кадр не портит.
void test_frame_with_trailing_space() {
  uint32_t d[80];
  size_t n = remote_frame(0x32, 0x02, false, 0, d);
  d[n++] = 12000;
  TEST_ASSERT_EQUAL_STRING("ir: nec 0x32 0x02 t=1", line(d, n, 1).c_str());
}

// «Код повтора и мусор»
void test_repeat_and_garbage() {
  nec_frame_t f;
  nec_encode_repeat(&f);
  uint32_t d[8];
  for (int i = 0; i < f.count; i++) d[i] = f.half_us[i] / 2 * (i == 1 ? 115 : 90) / 100;
  TEST_ASSERT_EQUAL_STRING("ir: повтор t=110", line(d, f.count, 110).c_str());

  uint32_t junk[9] = {700, 3100, 1500, 200, 9100, 800, 2500, 400, 300};
  TEST_ASSERT_EQUAL_STRING("ir: неразобрано 5 t=300", line(junk, 9, 300).c_str());
}

// Неверная инверсия команды — неразобрано.
void test_bad_command_inverse() {
  uint32_t d[80];
  size_t n = remote_frame(0x32, 0x02, false, 0, d);
  // бит 24 (первый бит ~команды) из 1 в 0: пауза бита — d[2 + 2*24 + 1]
  d[3 + 2 * 24] = 562;
  TEST_ASSERT_EQUAL_STRING("ir: неразобрано 34 t=0", line(d, n, 0).c_str());
}

// Длительность за пределами ±25 % — неразобрано.
void test_out_of_tolerance() {
  uint32_t d[80];
  size_t n = remote_frame(0x32, 0x02, false, 0, d);
  d[2] = 562 * 130 / 100;
  TEST_ASSERT_EQUAL(core::IrFrame::Unparsed, core::nec_decode(d, n).kind);
  d[2] = 562 * 124 / 100;
  TEST_ASSERT_EQUAL(core::IrFrame::Nec, core::nec_decode(d, n).kind);
}

void test_filter_merges_glitch() {
  // метка 9000, помеха: пауза 40 и метка 30, дальше пауза 4500
  uint32_t d[6] = {4000, 40, 4960, 4500, 560, 1690};
  size_t n = core::ir_filter(d, 6, 100);
  TEST_ASSERT_EQUAL(4, n);
  TEST_ASSERT_EQUAL_UINT32(9000, d[0]);
  TEST_ASSERT_EQUAL_UINT32(4500, d[1]);
  TEST_ASSERT_EQUAL_UINT32(560, d[2]);
}

// `#ir on` / `#ir off` и время событий от `#ir on`.
void test_ir_on_off() {
  Rig *r = new Rig;
  r->j.now = 5000;
  r->io.send("#ir on\n");
  r->run();
  TEST_ASSERT_TRUE(r->ir.enabled());
  TEST_ASSERT_EQUAL_UINT32(250, r->ir.since(5250));
  r->io.send("#ir off\n#ir x\n");
  r->run();
  TEST_ASSERT_FALSE(r->ir.enabled());
  TEST_ASSERT_EQUAL_STRING("# ir: вкл\n# ir: выкл\n# ir: неизвестный режим: x\n", r->out.all.c_str());
  delete r;
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_frame_button_a);
  RUN_TEST(test_frame_shrunk);
  RUN_TEST(test_frame_16bit_address);
  RUN_TEST(test_frame_with_trailing_space);
  RUN_TEST(test_repeat_and_garbage);
  RUN_TEST(test_bad_command_inverse);
  RUN_TEST(test_out_of_tolerance);
  RUN_TEST(test_filter_merges_glitch);
  RUN_TEST(test_ir_on_off);
  return UNITY_END();
}
