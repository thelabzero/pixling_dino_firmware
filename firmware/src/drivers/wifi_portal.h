// Wi-Fi setup portal (ADR 005): dino's own temporary network + a tiny web page, so a phone can tell dino which home
// Wi-Fi to join. Soft AP "DINO-1234" (WPA2, the password is shown on dino's screen), a DNS server that answers every
// name with dino (so phones open the setup page by themselves) and a web server with four routes:
//   /  the page      /scan  networks seen (async)      /save  credentials (POST)      /status  connecting / done / failed
// Non-blocking: call loop() often. The pairing rules live in wifi/wifi_setup.* (pure, tested); this file is the radio.
#pragma once
#include <Arduino.h>
#include <DNSServer.h>
#include <WebServer.h>
#include "../wifi/wifi_setup.h"

class WifiPortal {
 public:
  bool start(const char* apName, const char* apPassword, wifisetup::Session& session);  // network + DNS + web up
  void stop();                                                                          // everything down, radio off
  void loop(uint32_t nowMs);                                                            // serve the page, feed the session
  bool running() const { return running_; }

 private:
  void routes();
  void sendPage();
  void sendScan();
  void sendSave();
  void sendStatus();
  void redirectHome();
  void startScan();
  void reply(int code, const char* json);

  WebServer web_{80};
  DNSServer dns_;
  wifisetup::Session* session_ = nullptr;
  bool running_ = false;
  bool scanning_ = false;
  uint32_t scanDoneMs_ = 0;
  wifisetup::Session::State lastState_ = wifisetup::Session::State::Off;
};
