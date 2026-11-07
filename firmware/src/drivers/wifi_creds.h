// Saved Wi-Fi credentials (flash, NVS). Written only when a setup succeeded, never per loop (flash wear).
// Stored as plain text: the chip has no flash encryption (ADR 005 says what that means).
#pragma once
#include <stddef.h>

namespace wificreds {
bool load(char* ssid, size_t ssidCap, char* pass, size_t passCap);  // false if nothing is saved
bool save(const char* ssid, const char* pass);
void clear();
}  // namespace wificreds
