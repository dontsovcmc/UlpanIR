// Спека stand-test «Питание после ключа командой #power».
#include <unity.h>

#include "../fakes/fakes.h"

static Rig *r;
void setUp() { r = new Rig; }
void tearDown() { delete r; }

// «Пороги»
void test_thresholds() {
  r->adc.mv = {3280, 40};
  r->io.send("#power\n#power\n");
  r->run();
  TEST_ASSERT_EQUAL_STRING("# power: вкл 3.28 В\n# power: выкл 0.04 В\n", r->out.all.c_str());
}

void test_threshold_edge() {
  r->adc.mv = {1500, 1499};
  r->io.send("#power\n#power\n");
  r->run();
  TEST_ASSERT_EQUAL_STRING("# power: вкл 1.50 В\n# power: выкл 1.50 В\n", r->out.all.c_str());
}

void test_rounding() {
  r->adc.mv = {3304, 3305, 0};
  r->io.send("#power\n#power\n#power\n");
  r->run();
  TEST_ASSERT_EQUAL_STRING("# power: вкл 3.30 В\n# power: вкл 3.31 В\n# power: выкл 0.00 В\n", r->out.all.c_str());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_thresholds);
  RUN_TEST(test_threshold_edge);
  RUN_TEST(test_rounding);
  return UNITY_END();
}
