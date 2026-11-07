// Run: cd firmware && pio test -e native
#include <unity.h>
#include <string.h>
#include "wifi/wifi_setup.h"

using namespace wifisetup;
using utility::WifiEvent;

void setUp() {}
void tearDown() {}

static WifiEvent next(Session& s) { WifiEvent e = WifiEvent::TimedOut; TEST_ASSERT_TRUE(s.take(e)); return e; }
static bool none(Session& s) { WifiEvent e; return !s.take(e); }

// ---- credentials
void test_credentials_ok_and_open_network() {
  TEST_ASSERT_TRUE(checkCredentials("Home", "password1") == Check::Ok);
  TEST_ASSERT_TRUE(checkCredentials("Cafe", "") == Check::Ok);          // open network
  TEST_ASSERT_TRUE(checkCredentials("Cafe", nullptr) == Check::Ok);
  TEST_ASSERT_TRUE(checkCredentials("Hyderabad 5G ✓", "12345678") == Check::Ok);  // UTF-8 names are fine
}

void test_credentials_rejects_bad_input() {
  TEST_ASSERT_TRUE(checkCredentials("", "password1") == Check::SsidEmpty);
  TEST_ASSERT_TRUE(checkCredentials(nullptr, "password1") == Check::SsidEmpty);
  TEST_ASSERT_TRUE(checkCredentials("Home", "short") == Check::PassTooShort);
  TEST_ASSERT_TRUE(checkCredentials("Home", "1234567") == Check::PassTooShort);
  char longSsid[40]; memset(longSsid, 'a', 33); longSsid[33] = 0;
  TEST_ASSERT_TRUE(checkCredentials(longSsid, "password1") == Check::SsidTooLong);
  longSsid[32] = 0;
  TEST_ASSERT_TRUE(checkCredentials(longSsid, "password1") == Check::Ok);   // exactly 32 bytes
  char longPass[70]; memset(longPass, 'p', 64); longPass[64] = 0;
  TEST_ASSERT_TRUE(checkCredentials("Home", longPass) == Check::PassTooLong);
  longPass[63] = 0;
  TEST_ASSERT_TRUE(checkCredentials("Home", longPass) == Check::Ok);        // exactly 63
  TEST_ASSERT_TRUE(checkCredentials("Ho\nme", "password1") == Check::BadChars);
  TEST_ASSERT_TRUE(checkCredentials("Home", "pass\twordx") == Check::BadChars);
}

void test_every_check_has_a_message() {
  for (int i = 1; i <= (int)Check::Busy; i++) TEST_ASSERT_TRUE(strlen(checkMessage((Check)i)) > 10);  // every error says something
}

// ---- form decoding
void test_form_value_decodes_plus_and_percent() {
  char v[40];
  const char* body = "ssid=My+Home+Wi-Fi&pass=a%26b%3Dc%2B1&x=1";
  TEST_ASSERT_TRUE(formValue(body, "ssid", v, sizeof v)); TEST_ASSERT_EQUAL_STRING("My Home Wi-Fi", v);
  TEST_ASSERT_TRUE(formValue(body, "pass", v, sizeof v)); TEST_ASSERT_EQUAL_STRING("a&b=c+1", v);
  TEST_ASSERT_TRUE(formValue(body, "x", v, sizeof v)); TEST_ASSERT_EQUAL_STRING("1", v);
}

void test_form_value_utf8_and_edge_cases() {
  char v[40];
  TEST_ASSERT_TRUE(formValue("ssid=caf%C3%A9", "ssid", v, sizeof v)); TEST_ASSERT_EQUAL_STRING("caf\xC3\xA9", v);
  TEST_ASSERT_TRUE(formValue("pass=", "pass", v, sizeof v)); TEST_ASSERT_EQUAL_STRING("", v);          // empty value
  TEST_ASSERT_FALSE(formValue("ssid=a", "pass", v, sizeof v));                                         // missing key
  TEST_ASSERT_FALSE(formValue("xssid=a", "ssid", v, sizeof v));                                        // key must match whole name
  TEST_ASSERT_FALSE(formValue("ssid2=a&x=b", "ssid", v, sizeof v));
  TEST_ASSERT_TRUE(formValue("ssid=100%&pass=1", "ssid", v, sizeof v)); TEST_ASSERT_EQUAL_STRING("100%", v);   // lone % is kept
  TEST_ASSERT_TRUE(formValue("ssid=a%2", "ssid", v, sizeof v)); TEST_ASSERT_EQUAL_STRING("a%2", v);            // truncated escape kept
  TEST_ASSERT_TRUE(formValue("ssid=a%zz", "ssid", v, sizeof v)); TEST_ASSERT_EQUAL_STRING("a%zz", v);
}

void test_form_value_refuses_what_does_not_fit() {
  char v[4];
  TEST_ASSERT_TRUE(formValue("k=abc", "k", v, sizeof v));            // 3 chars + NUL fits
  TEST_ASSERT_FALSE(formValue("k=abcd", "k", v, sizeof v));          // 4 chars does not
  TEST_ASSERT_EQUAL_STRING("", v);
  TEST_ASSERT_FALSE(formValue("k=abc", "k", v, 0));
  TEST_ASSERT_FALSE(formValue(nullptr, "k", v, sizeof v));
}

// ---- JSON strings for the network list
void test_json_string_escapes() {
  char o[64];
  TEST_ASSERT_TRUE(jsonString("Home", o, sizeof o)); TEST_ASSERT_EQUAL_STRING("\"Home\"", o);
  TEST_ASSERT_TRUE(jsonString("a\"b\\c", o, sizeof o)); TEST_ASSERT_EQUAL_STRING("\"a\\\"b\\\\c\"", o);
  TEST_ASSERT_TRUE(jsonString("x\ny\x01", o, sizeof o)); TEST_ASSERT_EQUAL_STRING("\"x\\u000ay\\u0001\"", o);
  TEST_ASSERT_TRUE(jsonString("caf\xC3\xA9", o, sizeof o)); TEST_ASSERT_EQUAL_STRING("\"caf\xC3\xA9\"", o);   // UTF-8 untouched
  TEST_ASSERT_TRUE(jsonString("", o, sizeof o)); TEST_ASSERT_EQUAL_STRING("\"\"", o);
  TEST_ASSERT_TRUE(jsonString(nullptr, o, sizeof o)); TEST_ASSERT_EQUAL_STRING("\"\"", o);
  TEST_ASSERT_TRUE(jsonString("</script>", o, sizeof o));                                        // nothing HTML-special is needed in JSON we serve as JSON
}

void test_json_string_never_overflows() {
  char o[8];
  TEST_ASSERT_TRUE(jsonString("abcde", o, sizeof o));            // "abcde" = 7 chars + NUL = 8: fits exactly
  TEST_ASSERT_EQUAL(7, (int)strlen(o));
  TEST_ASSERT_FALSE(jsonString("abcdef", o, sizeof o));
  TEST_ASSERT_EQUAL_STRING("", o);
  TEST_ASSERT_FALSE(jsonString("a\"", o, 4));                     // the escape does not fit
  TEST_ASSERT_FALSE(jsonString("a", o, 2));
  TEST_ASSERT_FALSE(jsonString("a", nullptr, 8));
}

// ---- code and name
void test_code_is_ten_characters_without_lookalikes() {
  char c[CODE_LEN + 1];
  makeCode(0, c); TEST_ASSERT_EQUAL_STRING("AAAAAAAAAA", c);
  makeCode(0xFFFFFFFFFFFFFFFFull, c); TEST_ASSERT_EQUAL_STRING("9999999999", c);
  makeCode(1, c); TEST_ASSERT_EQUAL_STRING("BAAAAAAAAA", c);
  // every possible 5-bit value maps to a distinct, readable character: no 0, O, 1, I, lower case or symbol
  bool seen[128] = {};
  for (int v = 0; v < 32; v++) {
    makeCode((uint64_t)v, c);
    TEST_ASSERT_TRUE(c[0] >= '2' && c[0] <= 'Z');
    TEST_ASSERT_TRUE(c[0] != 'O' && c[0] != 'I' && c[0] != '0' && c[0] != '1');
    TEST_ASSERT_FALSE(seen[(int)c[0]]);
    seen[(int)c[0]] = true;
  }
  makeCode(0x123456789ABCDEFull, c);
  TEST_ASSERT_EQUAL(CODE_LEN, (int)strlen(c));
}

void test_ap_name() {
  char n[10];
  makeApName(7, n); TEST_ASSERT_EQUAL_STRING("DINO-0007", n);
  makeApName(4821, n); TEST_ASSERT_EQUAL_STRING("DINO-4821", n);
  makeApName(65535, n); TEST_ASSERT_EQUAL_STRING("DINO-5535", n);          // always 4 digits
}

// ---- session
void test_happy_path() {
  Session s; s.begin(1000);
  TEST_ASSERT_TRUE(s.state() == Session::State::Waiting); TEST_ASSERT_TRUE(s.active());
  s.onStations(1, 2000);
  TEST_ASSERT_TRUE(next(s) == WifiEvent::PhoneJoined); TEST_ASSERT_TRUE(s.state() == Session::State::PhoneJoined);
  TEST_ASSERT_TRUE(s.submit("Home", "password1", 3000) == Check::Ok);
  TEST_ASSERT_TRUE(next(s) == WifiEvent::Connecting);
  TEST_ASSERT_EQUAL_STRING("Home", s.ssid()); TEST_ASSERT_EQUAL_STRING("password1", s.pass());
  s.onConnectResult(true, 6000);
  TEST_ASSERT_TRUE(next(s) == WifiEvent::Connected);
  TEST_ASSERT_TRUE(s.state() == Session::State::Done); TEST_ASSERT_FALSE(s.active());
  TEST_ASSERT_TRUE(none(s));
}

void test_bad_submit_changes_nothing() {
  Session s; s.begin(0); s.onStations(1, 10); next(s);
  TEST_ASSERT_TRUE(s.submit("Home", "short", 20) == Check::PassTooShort);
  TEST_ASSERT_TRUE(s.state() == Session::State::PhoneJoined);
  TEST_ASSERT_TRUE(none(s));
  TEST_ASSERT_TRUE(s.submit("Home", "password1", 30) == Check::Ok);
  TEST_ASSERT_TRUE(s.submit("Other", "password2", 31) == Check::Busy);      // already trying
  TEST_ASSERT_EQUAL_STRING("Home", s.ssid());
}

void test_failed_connect_goes_back_and_can_retry() {
  Session s; s.begin(0); s.onStations(1, 10); next(s);
  s.submit("Home", "wrongpass1", 100); next(s);
  s.onConnectResult(false, 3000);
  TEST_ASSERT_TRUE(next(s) == WifiEvent::Failed);
  TEST_ASSERT_TRUE(s.state() == Session::State::PhoneJoined);               // the phone is still there
  TEST_ASSERT_TRUE(s.lastFailed());
  TEST_ASSERT_TRUE(s.submit("Home", "rightpass1", 4000) == Check::Ok);
  TEST_ASSERT_FALSE(s.lastFailed());
  next(s);
  s.onConnectResult(true, 5000);
  TEST_ASSERT_TRUE(next(s) == WifiEvent::Connected);
}

void test_connect_timeout() {
  Timing t; t.connectMs = 20000;
  Session s; s.configure(t); s.begin(0); s.onStations(1, 10); next(s);
  s.submit("Home", "password1", 1000); next(s);
  s.tick(20999); TEST_ASSERT_TRUE(none(s));
  s.tick(21000);
  TEST_ASSERT_TRUE(next(s) == WifiEvent::Failed);
  TEST_ASSERT_TRUE(s.state() == Session::State::PhoneJoined);
}

void test_phone_leaves_and_returns() {
  Session s; s.begin(0);
  s.onStations(1, 100); TEST_ASSERT_TRUE(next(s) == WifiEvent::PhoneJoined);
  s.onStations(1, 200); TEST_ASSERT_TRUE(none(s));                          // still one phone: no new event
  s.onStations(0, 300); TEST_ASSERT_TRUE(next(s) == WifiEvent::PhoneLeft);
  TEST_ASSERT_TRUE(s.state() == Session::State::Waiting);
  s.onStations(2, 400); TEST_ASSERT_TRUE(next(s) == WifiEvent::PhoneJoined);
}

void test_failed_connect_without_phone_waits() {
  Session s; s.begin(0); s.onStations(1, 10); next(s);
  s.submit("Home", "password1", 100); next(s);
  s.onStations(0, 150);                                                     // phone dropped while connecting: ignored until result
  TEST_ASSERT_TRUE(s.state() == Session::State::Connecting);
  s.onConnectResult(false, 200); next(s);
  TEST_ASSERT_TRUE(s.state() == Session::State::Waiting);
}

void test_wait_timeout_and_life_extends_it() {
  Timing t; t.waitMs = 60000;
  Session s; s.configure(t); s.begin(0);
  s.tick(59999); TEST_ASSERT_TRUE(none(s));
  s.onStations(1, 50000); next(s);                                          // a phone joins: window restarts
  s.tick(109999); TEST_ASSERT_TRUE(none(s));
  s.tick(110000);
  TEST_ASSERT_TRUE(next(s) == WifiEvent::TimedOut);
  TEST_ASSERT_TRUE(s.state() == Session::State::Off);
}

void test_cancel_clears_everything() {
  Session s; s.begin(0); s.onStations(1, 10);
  s.cancel();
  TEST_ASSERT_TRUE(s.state() == Session::State::Off); TEST_ASSERT_TRUE(none(s));
  s.tick(10000000); TEST_ASSERT_TRUE(none(s));                              // off: no time-out events
  TEST_ASSERT_TRUE(s.submit("Home", "password1", 5) == Check::Busy);        // and no submit
}

void test_begin_forgets_a_previous_session() {
  Session s; s.begin(0); s.onStations(1, 10); s.submit("Home", "password1", 20);
  s.begin(1000);
  TEST_ASSERT_TRUE(s.state() == Session::State::Waiting);
  TEST_ASSERT_EQUAL_STRING("", s.ssid()); TEST_ASSERT_EQUAL_STRING("", s.pass());
  TEST_ASSERT_TRUE(none(s)); TEST_ASSERT_FALSE(s.lastFailed());
}

void test_clock_wrap() {  // millis() wraps after 49 days: time-outs must still work
  Session s; Timing t; t.waitMs = 60000; s.configure(t);
  s.begin(0xFFFFFFFFu - 1000);
  s.tick(0xFFFFFFFFu - 1000 + 59000); TEST_ASSERT_TRUE(none(s));
  s.tick(59000);                                                            // 1001 ms to the wrap + 59000 = 60001 ms after begin
  TEST_ASSERT_TRUE(next(s) == WifiEvent::TimedOut);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_credentials_ok_and_open_network);
  RUN_TEST(test_credentials_rejects_bad_input);
  RUN_TEST(test_every_check_has_a_message);
  RUN_TEST(test_form_value_decodes_plus_and_percent);
  RUN_TEST(test_form_value_utf8_and_edge_cases);
  RUN_TEST(test_form_value_refuses_what_does_not_fit);
  RUN_TEST(test_json_string_escapes);
  RUN_TEST(test_json_string_never_overflows);
  RUN_TEST(test_code_is_ten_characters_without_lookalikes);
  RUN_TEST(test_ap_name);
  RUN_TEST(test_happy_path);
  RUN_TEST(test_bad_submit_changes_nothing);
  RUN_TEST(test_failed_connect_goes_back_and_can_retry);
  RUN_TEST(test_connect_timeout);
  RUN_TEST(test_phone_leaves_and_returns);
  RUN_TEST(test_failed_connect_without_phone_waits);
  RUN_TEST(test_wait_timeout_and_life_extends_it);
  RUN_TEST(test_cancel_clears_everything);
  RUN_TEST(test_begin_forgets_a_previous_session);
  RUN_TEST(test_clock_wrap);
  return UNITY_END();
}
