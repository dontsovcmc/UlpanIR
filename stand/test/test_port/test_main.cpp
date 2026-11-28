// Спека stand-port: команды и ответы, лог пульта в том же порту, `#info`.
#include <unity.h>

#include "../fakes/fakes.h"

static Rig *r;
void setUp() { r = new Rig; }
void tearDown() { delete r; }

// «Неизвестная команда»
void test_unknown_command() {
  r->io.send("#foo\n");
  r->run();
  TEST_ASSERT_EQUAL_STRING("# неизвестная команда: foo\n", r->out.all.c_str());
}

// «Слишком длинная команда»
void test_too_long_command() {
  r->j.now = 5000;
  r->io.send("#" + std::string(100, 'a') + "\n#info\n");
  r->run();
  TEST_ASSERT_EQUAL_STRING("# слишком длинная команда\n# ulpanir_stand 0.1.0 uptime=5\n", r->out.all.c_str());
}

// Ровно 64 байта — ещё команда.
void test_64_bytes_is_command() {
  r->io.send("#" + std::string(63, 'b') + "\r\n");
  r->run();
  TEST_ASSERT_EQUAL_STRING(("# неизвестная команда: " + std::string(63, 'b') + "\n").c_str(), r->out.all.c_str());
}

// «Версия и время работы»
void test_info() {
  r->j.now = 42000;
  r->io.send("#info\n");
  r->stand.step(0);
  TEST_ASSERT_EQUAL_STRING("# ulpanir_stand 0.1.0 uptime=42\n", r->out.all.c_str());
}

// «Ответ посреди строки лога»
void test_reply_mid_log_line() {
  r->console.log((const uint8_t *)"btn A", 5);
  r->io.send("#info\n");
  r->stand.step(0);
  r->console.log((const uint8_t *)" pressed\n", 9);
  TEST_ASSERT_EQUAL_STRING("btn A\n# ulpanir_stand 0.1.0 uptime=0\n pressed\n", r->out.all.c_str());
}

// После целой строки лога лишний `\n` не нужен.
void test_reply_after_full_log_line() {
  r->console.log((const uint8_t *)"boot\n", 5);
  r->console.reply("x");
  TEST_ASSERT_EQUAL_STRING("boot\n# x\n", r->out.all.c_str());
}

// Команды выполняются по одной, в порядке прихода.
void test_commands_in_order() {
  r->io.send("#info\n#lines off\n#bar\n");
  r->run();
  TEST_ASSERT_EQUAL_STRING("# ulpanir_stand 0.1.0 uptime=0\n# lines: отпущены\n# неизвестная команда: bar\n",
                           r->out.all.c_str());
}

void test_lines_unknown_mode() {
  r->io.send("#lines on\n");
  r->run();
  TEST_ASSERT_EQUAL_STRING("# lines: неизвестный режим: on\n", r->out.all.c_str());
}

void test_lines_off_releases_all() {
  r->io.send("#lines off\n");
  r->run();
  TEST_ASSERT_TRUE(r->j.find("release_all") >= 0);
}

// Пустые строки молча пропускаются: `\r\n` от терминала не даёт лишних ответов.
void test_empty_line_ignored() {
  r->io.send("\n\r\n#info\n");
  r->run();
  TEST_ASSERT_EQUAL_STRING("# ulpanir_stand 0.1.0 uptime=0\n", r->out.all.c_str());
}

// Диагностика задачи 8.1: самый большой свободный кусок кучи и вся свободная куча.
void test_heap() {
  r->io.send("#heap\n");
  r->run();
  TEST_ASSERT_EQUAL_STRING("# heap: max=331776 free=401408\n", r->out.all.c_str());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_unknown_command);
  RUN_TEST(test_too_long_command);
  RUN_TEST(test_64_bytes_is_command);
  RUN_TEST(test_info);
  RUN_TEST(test_reply_mid_log_line);
  RUN_TEST(test_reply_after_full_log_line);
  RUN_TEST(test_commands_in_order);
  RUN_TEST(test_lines_unknown_mode);
  RUN_TEST(test_lines_off_releases_all);
  RUN_TEST(test_empty_line_ignored);
  RUN_TEST(test_heap);
  return UNITY_END();
}
