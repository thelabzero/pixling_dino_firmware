// Wi-Fi setup logic ("connect your dino", ADR 005): everything about pairing that does not touch the radio.
// Pure C++ (no Arduino calls), tested in env:native.
//
//   - checkCredentials(): is this a Wi-Fi name / password the chip can use?
//   - formValue():        reads one field out of the setup page's POST body (percent-decoding).
//   - makeCode / makeApName: the temporary network's name ("DINO-1234") and its password (10 letters and digits, shown on the OLED).
//   - Session:            the pairing state machine: waiting -> phone joined -> connecting -> done, with time-outs.
//                         The radio module feeds it facts (stations, connect result) and reads events out.
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "../utility/machine.h"

namespace wifisetup {

constexpr size_t SSID_MAX = 32;          // 802.11 limit, in bytes
constexpr size_t PASSPHRASE_MIN = 8, PASSPHRASE_MAX = 63;  // WPA2-PSK passphrase; empty = open network

enum class Check : uint8_t { Ok, SsidEmpty, SsidTooLong, PassTooShort, PassTooLong, BadChars, Busy };
Check checkCredentials(const char* ssid, const char* pass);
const char* checkMessage(Check c);  // one short sentence for the setup page

// Value of `key` in an application/x-www-form-urlencoded body ("ssid=My+Wi-Fi&pass=a%26b"), decoded.
// False if the key is missing or the value does not fit in `cap` bytes (including the NUL).
bool formValue(const char* body, const char* key, char* out, size_t cap);

// `in` as a quoted JSON string (quotes, backslashes and control characters escaped; UTF-8 passes through), for the
// network list sent to the page. False if it does not fit in `cap` bytes (out is then an empty string "").
bool jsonString(const char* in, char* out, size_t cap);

// The setup network's WPA2 password: 10 characters from a 32-character alphabet without look-alikes (no 0 O 1 I),
// 5 bits each = 50 bits taken from `random`. An 8-digit code (27 bits) could be cracked offline from a recorded handshake.
constexpr int CODE_LEN = 10;
void makeCode(uint64_t random, char out[CODE_LEN + 1]);
void makeApName(uint16_t suffix, char out[10]);   // "DINO-" + 4 digits

struct Timing {
  uint32_t waitMs = 5UL * 60000;    // nobody finishes the setup within this long of the last sign of life: give up
  uint32_t connectMs = 20000;       // the home Wi-Fi did not accept the credentials within this long: failed
};

class Session {
 public:
  enum class State : uint8_t { Off, Waiting, PhoneJoined, Connecting, Done };

  void configure(const Timing& t) { timing_ = t; }
  void begin(uint32_t nowMs);                         // open the setup (network goes up)
  void cancel();                                      // close it (user left, or the radio module stopped)
  void onStations(int count, uint32_t nowMs);         // phones joined to the setup network right now
  Check submit(const char* ssid, const char* pass, uint32_t nowMs);  // the page sent credentials: Ok = trying now
  void onConnectResult(bool ok, uint32_t nowMs);      // the chip joined the home Wi-Fi (or refused)
  void tick(uint32_t nowMs);                          // time-outs
  bool take(utility::WifiEvent& e);                   // next event for the brain

  State state() const { return state_; }
  bool active() const { return state_ != State::Off && state_ != State::Done; }
  bool lastFailed() const { return failed_; }         // the page shows "did not connect" until the next try
  const char* ssid() const { return ssid_; }          // valid once submit() returned Ok
  const char* pass() const { return pass_; }
  const char* stateName() const;

 private:
  void push(utility::WifiEvent e);
  Timing timing_;
  State state_ = State::Off;
  char ssid_[SSID_MAX + 1] = "";
  char pass_[PASSPHRASE_MAX + 1] = "";
  int stations_ = 0;
  bool failed_ = false;
  uint32_t lastLifeMs_ = 0, connectStartMs_ = 0;
  utility::WifiEvent q_[4];
  uint8_t qHead_ = 0, qLen_ = 0;
};

}  // namespace wifisetup
