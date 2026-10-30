// Run: cd firmware && pio test -e native
#include <unity.h>
#include "input/serial_touch.h"

using namespace input;

void setUp() {}
void tearDown() {}

void test_letters() {
  TEST_ASSERT_EQUAL((int)Event::Tap, (int)touchFromChar('t'));
  TEST_ASSERT_EQUAL((int)Event::DoubleTap, (int)touchFromChar('d'));
  TEST_ASSERT_EQUAL((int)Event::LongPress, (int)touchFromChar('L'));
  TEST_ASSERT_EQUAL((int)Event::Hold, (int)touchFromChar('h'));
  TEST_ASSERT_EQUAL((int)Event::None, (int)touchFromChar('\n'));
  TEST_ASSERT_EQUAL((int)Event::None, (int)touchFromChar('x'));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_letters);
  return UNITY_END();
}
