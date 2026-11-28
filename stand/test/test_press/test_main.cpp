// Спеки stand-test «Нажатие кнопок командой #press» и stand-port «Отпустить всё».
#include <unity.h>

#include "../fakes/fakes.h"

static Rig *r;
void setUp() { r = new Rig; }
void tearDown() { delete r; }

// «Короткое нажатие A»
void test_short_press_a() {
  r->io.send("#press a 100\n");
  r->run();
  int p = r->j.find("press 1");
  int rel = r->j.find("release", p);
  TEST_ASSERT_TRUE(p >= 0);
  TEST_ASSERT_TRUE(rel > p);
  TEST_ASSERT_GREATER_OR_EQUAL_UINT32(100, r->j.time_of(rel) - r->j.time_of(p));
  TEST_ASSERT_EQUAL_STRING("# press: a 100\n", r->out.all.c_str());
  TEST_ASSERT_GREATER_OR_EQUAL_UINT32(r->j.time_of(rel), r->out.time_of("# press: a 100"));
  TEST_ASSERT_EQUAL(-1, r->j.find("press 2"));
  TEST_ASSERT_EQUAL(-1, r->j.find("press 3"));
}

void test_press_both_at_once() {
  r->io.send("#press ab 500\n");
  r->run();
  TEST_ASSERT_TRUE(r->j.find("press 3") >= 0);
  TEST_ASSERT_EQUAL_STRING("# press: ab 500\n", r->out.all.c_str());
}

void test_press_b() {
  r->io.send("#press b 1\n");
  r->run();
  TEST_ASSERT_TRUE(r->j.find("press 2") >= 0);
  TEST_ASSERT_EQUAL_STRING("# press: b 1\n", r->out.all.c_str());
}

// «Длительность вне пределов»
void test_duration_out_of_range() {
  r->io.send("#press b 20000\n");
  r->run();
  TEST_ASSERT_EQUAL_STRING("# press: неизвестная длительность: 20000\n", r->out.all.c_str());
  TEST_ASSERT_EQUAL(-1, r->j.find("press"));
}

void test_bad_durations() {
  r->io.send("#press a 0\n#press a 1x\n#press a\n#press a 10001\n");
  r->run();
  TEST_ASSERT_EQUAL_STRING(
      "# press: неизвестная длительность: 0\n# press: неизвестная длительность: 1x\n"
      "# press: неизвестная длительность: \n# press: неизвестная длительность: 10001\n",
      r->out.all.c_str());
  TEST_ASSERT_EQUAL(-1, r->j.find("press"));
}

void test_unknown_button() {
  r->io.send("#press c 100\n");
  r->run();
  TEST_ASSERT_EQUAL_STRING("# press: неизвестная кнопка: c\n", r->out.all.c_str());
  TEST_ASSERT_EQUAL(-1, r->j.find("press"));
}

void test_max_duration() {
  r->io.send("#press a 10000\n");
  r->run();
  TEST_ASSERT_EQUAL_STRING("# press: a 10000\n", r->out.all.c_str());
}

// «Отпустить всё»: `#lines off` посреди `#press a 5000`.
void test_lines_off_interrupts_press() {
  r->io.send("#press a 5000\n");
  r->io.send("#lines off\n", 1000);
  r->run();
  int rel = r->j.find("release_all");
  TEST_ASSERT_TRUE(rel >= 0);
  TEST_ASSERT_LESS_THAN_UINT32(1200, r->j.time_of(rel));
  TEST_ASSERT_EQUAL(0, r->lines.low);
  TEST_ASSERT_EQUAL_STRING("# lines: отпущены\n", r->out.all.c_str());
}

// Команды, пришедшие во время нажатия, не теряются и выполняются после него.
void test_commands_during_press_are_queued() {
  r->io.send("#press a 300\n");
  r->io.send("#info\n", 100);
  r->run();
  TEST_ASSERT_EQUAL_STRING("# press: a 300\n# ulpanir_stand 0.1.0 uptime=0\n", r->out.all.c_str());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_short_press_a);
  RUN_TEST(test_press_both_at_once);
  RUN_TEST(test_press_b);
  RUN_TEST(test_duration_out_of_range);
  RUN_TEST(test_bad_durations);
  RUN_TEST(test_unknown_button);
  RUN_TEST(test_max_duration);
  RUN_TEST(test_lines_off_interrupts_press);
  RUN_TEST(test_commands_during_press_are_queued);
  return UNITY_END();
}
