#include "wifi_creds.h"
#include <Preferences.h>
#include <string.h>

namespace wificreds {

static const char* NS = "dino_wifi";

bool load(char* ssid, size_t ssidCap, char* pass, size_t passCap) {
  Preferences p;
  if (!p.begin(NS, true)) return false;  // read-only; fails before anything was ever saved
  const bool has = p.isKey("ssid");
  if (has) {
    p.getString("ssid", ssid, ssidCap);
    p.getString("pass", pass, passCap);
  }
  p.end();
  return has && ssid[0];
}

bool save(const char* ssid, const char* pass) {
  if (!ssid) return false;
  if (!pass) pass = "";
  Preferences p;
  if (!p.begin(NS, false)) return false;
  // putString returns the bytes written: compare with the length (an open network has an empty password)
  const bool ok = p.putString("ssid", ssid) == strlen(ssid) && p.putString("pass", pass) == strlen(pass);
  p.end();
  return ok;
}

void clear() {
  Preferences p;
  if (p.begin(NS, false)) { p.clear(); p.end(); }
}

}  // namespace wificreds
