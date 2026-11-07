#include "wifi_setup.h"
#include <string.h>

namespace wifisetup {

using utility::WifiEvent;

static bool printable(const char* s) {
  for (; *s; s++) if ((unsigned char)*s < 0x20 || (unsigned char)*s == 0x7F) return false;
  return true;
}

Check checkCredentials(const char* ssid, const char* pass) {
  if (!ssid || !*ssid) return Check::SsidEmpty;
  if (strlen(ssid) > SSID_MAX) return Check::SsidTooLong;
  if (!pass) pass = "";
  if (!printable(ssid) || !printable(pass)) return Check::BadChars;
  const size_t n = strlen(pass);
  if (n == 0) return Check::Ok;  // open network
  if (n < PASSPHRASE_MIN) return Check::PassTooShort;
  if (n > PASSPHRASE_MAX) return Check::PassTooLong;
  return Check::Ok;
}

const char* checkMessage(Check c) {
  switch (c) {
    case Check::Ok: return "OK";
    case Check::SsidEmpty: return "Pick your Wi-Fi network first.";
    case Check::SsidTooLong: return "A Wi-Fi name can be 32 characters at most.";
    case Check::PassTooShort: return "A Wi-Fi password has at least 8 characters.";
    case Check::PassTooLong: return "A Wi-Fi password has at most 63 characters.";
    case Check::BadChars: return "The name or password has a character dino cannot use.";
    case Check::Busy: return "Dino is busy. Try again in a moment.";
  }
  return "?";
}

static int hexVal(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

bool formValue(const char* body, const char* key, char* out, size_t cap) {
  if (!body || !key || !out || cap == 0) return false;
  const size_t kl = strlen(key);
  const char* p = body;
  while (*p) {
    const char* end = strchr(p, '&');
    const size_t fl = end ? (size_t)(end - p) : strlen(p);  // this field: p .. p+fl
    if (fl > kl && p[kl] == '=' && strncmp(p, key, kl) == 0) {
      size_t n = 0;
      for (const char* v = p + kl + 1; v < p + fl; v++) {
        char c = *v;
        if (c == '+') c = ' ';
        else if (c == '%' && v + 2 < p + fl && hexVal(v[1]) >= 0 && hexVal(v[2]) >= 0) {  // both hex digits inside this field
          c = (char)(hexVal(v[1]) * 16 + hexVal(v[2]));
          v += 2;
        }
        if (n + 1 >= cap) { out[0] = 0; return false; }
        out[n++] = c;
      }
      out[n] = 0;
      return true;
    }
    if (!end) break;
    p = end + 1;
  }
  out[0] = 0;
  return false;
}

bool jsonString(const char* in, char* out, size_t cap) {
  if (!out || cap < 3) { if (out && cap) out[0] = 0; return false; }
  size_t n = 0;
  out[n++] = '"';
  for (; in && *in; in++) {
    const unsigned char c = (unsigned char)*in;
    char tmp[7];
    size_t len = 0;
    if (c == '"' || c == '\\') { tmp[0] = '\\'; tmp[1] = (char)c; len = 2; }
    else if (c < 0x20 || c == 0x7F) {
      static const char HEX[] = "0123456789abcdef";
      memcpy(tmp, "\\u00", 4); tmp[4] = HEX[c >> 4]; tmp[5] = HEX[c & 15]; len = 6;
    } else { tmp[0] = (char)c; len = 1; }
    if (n + len + 2 > cap) { out[0] = 0; return false; }  // room for the closing quote and the NUL
    memcpy(out + n, tmp, len); n += len;
  }
  out[n++] = '"';
  out[n] = 0;
  return true;
}

void makeCode(uint64_t random, char out[CODE_LEN + 1]) {
  static const char ALPHABET[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";  // 32 characters
  for (int i = 0; i < CODE_LEN; i++) out[i] = ALPHABET[(random >> (5 * i)) & 31];
  out[CODE_LEN] = 0;
}

void makeApName(uint16_t suffix, char out[10]) {
  uint32_t v = suffix % 10000;
  memcpy(out, "DINO-", 5);
  for (int i = 8; i >= 5; i--) { out[i] = (char)('0' + v % 10); v /= 10; }
  out[9] = 0;
}

// ---------------------------------------------------------------- session
void Session::push(WifiEvent e) {
  if (qLen_ < 4) q_[(qHead_ + qLen_++) % 4] = e;
}

bool Session::take(WifiEvent& e) {
  if (!qLen_) return false;
  e = q_[qHead_];
  qHead_ = (qHead_ + 1) % 4; qLen_--;
  return true;
}

void Session::begin(uint32_t now) {
  state_ = State::Waiting;
  ssid_[0] = pass_[0] = 0;
  stations_ = 0;
  failed_ = false;
  lastLifeMs_ = now;
  qHead_ = qLen_ = 0;
}

void Session::cancel() {
  state_ = State::Off;
  stations_ = 0;
  qHead_ = qLen_ = 0;
}

void Session::onStations(int count, uint32_t now) {
  stations_ = count < 0 ? 0 : count;
  if (stations_ > 0) lastLifeMs_ = now;
  if (state_ == State::Waiting && stations_ > 0) { state_ = State::PhoneJoined; push(WifiEvent::PhoneJoined); }
  else if (state_ == State::PhoneJoined && stations_ == 0) { state_ = State::Waiting; push(WifiEvent::PhoneLeft); }
}

Check Session::submit(const char* ssid, const char* pass, uint32_t now) {
  if (state_ != State::Waiting && state_ != State::PhoneJoined) return Check::Busy;
  const Check c = checkCredentials(ssid, pass);
  if (c != Check::Ok) return c;
  strncpy(ssid_, ssid, SSID_MAX); ssid_[SSID_MAX] = 0;
  strncpy(pass_, pass ? pass : "", PASSPHRASE_MAX); pass_[PASSPHRASE_MAX] = 0;
  failed_ = false;
  state_ = State::Connecting;
  connectStartMs_ = lastLifeMs_ = now;
  push(WifiEvent::Connecting);
  return Check::Ok;
}

void Session::onConnectResult(bool ok, uint32_t now) {
  if (state_ != State::Connecting) return;
  lastLifeMs_ = now;
  if (ok) { state_ = State::Done; push(WifiEvent::Connected); return; }
  failed_ = true;
  state_ = stations_ > 0 ? State::PhoneJoined : State::Waiting;
  push(WifiEvent::Failed);
}

void Session::tick(uint32_t now) {
  if (state_ == State::Connecting) {
    if ((int32_t)(now - connectStartMs_) >= (int32_t)timing_.connectMs) onConnectResult(false, now);
  } else if (state_ == State::Waiting || state_ == State::PhoneJoined) {
    if ((int32_t)(now - lastLifeMs_) >= (int32_t)timing_.waitMs) { state_ = State::Off; push(WifiEvent::TimedOut); }
  }
}

const char* Session::stateName() const {
  static const char* const N[] = {"off", "waiting", "phone_joined", "connecting", "done"};
  return N[(int)state_];
}

}  // namespace wifisetup
