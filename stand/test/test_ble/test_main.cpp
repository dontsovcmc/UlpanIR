// Спека stand-test «Подключение к пульту по BLE», «Чтение и запись характеристик по BLE».
#include <unity.h>

#include "../fakes/fakes.h"

static Rig *r;
void setUp() { r = new Rig; }
void tearDown() { delete r; }

// «Без подключения»
void test_without_connection() {
  r->io.send("#ble read config\n#ble write ping 00\n");
  r->run();
  TEST_ASSERT_EQUAL_STRING("# ble: нет подключения\n# ble: нет подключения\n", r->out.all.c_str());
}

// «Запись кодов» на поддельном BLE.
void test_write_codes() {
  r->io.send("#ble connect\n#ble write config 0101320002013200030300\n#ble read config\n#ble close\n");
  r->run();
  TEST_ASSERT_EQUAL_STRING(
      "# ble: подключён IR-Remote-1A2B rssi=-61\n# ble: записано config\n"
      "# ble: config 0101320002013200030300\n# ble: отключён\n",
      r->out.all.c_str());
  TEST_ASSERT_FALSE(r->ble.connected());
}

void test_read_status() {
  r->io.send("#ble connect 3000\n#ble read status\n");
  r->run();
  TEST_ASSERT_EQUAL_STRING("# ble: подключён IR-Remote-1A2B rssi=-61\n# ble: status 01000100e40c\n",
                           r->out.all.c_str());
}

// «Пульт не в режиме настройки»: ответ через заданное время.
void test_not_found() {
  r->ble.found = false;
  r->io.send("#ble connect 3000\n");
  r->stand.step(0);
  TEST_ASSERT_EQUAL_STRING("# ble: не найден\n", r->out.all.c_str());
  TEST_ASSERT_EQUAL_UINT32(3000, r->out.time_of("не найден"));
}

void test_rejected() {
  r->ble.reject = true;
  r->io.send("#ble connect\n#ble write test 00\n");
  r->run();
  TEST_ASSERT_EQUAL_STRING("# ble: подключён IR-Remote-1A2B rssi=-61\n# ble: отклонено test\n", r->out.all.c_str());
}

void test_bad_arguments() {
  r->io.send(
      "#ble connect 0\n#ble connect 60001\n#ble read ping\n#ble write status 00\n#ble write config 0g\n"
      "#ble write config 123\n#ble write config\n#ble scan\n");
  r->run();
  TEST_ASSERT_EQUAL_STRING(
      "# ble: неизвестная длительность: 0\n# ble: неизвестная длительность: 60001\n"
      "# ble: неизвестная характеристика: ping\n# ble: неизвестная характеристика: status\n"
      "# ble: неверные данные: 0g\n# ble: неверные данные: 123\n# ble: неверные данные: \n"
      "# ble: неизвестная команда: scan\n",
      r->out.all.c_str());
}

void test_write_ping_bytes() {
  r->io.send("#ble connect\n#ble write ping 00\n");
  r->run();
  TEST_ASSERT_EQUAL_STRING(std::string("2:\0", 3).c_str(), r->ble.last_write.c_str());
  TEST_ASSERT_EQUAL(3, (int)r->ble.last_write.size());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_without_connection);
  RUN_TEST(test_write_codes);
  RUN_TEST(test_read_status);
  RUN_TEST(test_not_found);
  RUN_TEST(test_rejected);
  RUN_TEST(test_bad_arguments);
  RUN_TEST(test_write_ping_bytes);
  return UNITY_END();
}
