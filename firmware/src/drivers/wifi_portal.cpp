#include "wifi_portal.h"
#include <WiFi.h>
#include "config.h"

using wifisetup::Check;
using wifisetup::Session;

// The setup page: one file, no outside resources (a phone on dino's network has no internet).
// Network names are put into the page with textContent, never as HTML.
static const char PAGE[] PROGMEM = R"HTML(<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>Connect your Dino</title>
<style>
:root{color-scheme:light dark}
body{margin:0;font:16px/1.5 system-ui,sans-serif;background:#111;color:#f2f2ee}
main{max-width:420px;margin:0 auto;padding:24px 16px}
h1{font-size:24px;margin:0 0 4px}p{margin:0 0 16px;color:#b9b8b0}
label{display:block;margin:16px 0 4px;font-weight:600}
select,input[type=text],input[type=password]{width:100%;box-sizing:border-box;padding:12px;font:inherit;border-radius:8px;border:1px solid #555;background:#1c1c1c;color:inherit}
button{width:100%;margin-top:20px;padding:14px;font:inherit;font-weight:700;border:0;border-radius:8px;background:#f2f2ee;color:#111}
button.alt{margin-top:8px;padding:8px;background:none;color:#b9b8b0;font-weight:400;text-decoration:underline}
button:disabled{opacity:.5}
#msg{min-height:3em;margin-top:16px;color:#f2f2ee}small{color:#8a8980}
.row{display:flex;align-items:center;gap:8px;margin-top:8px;font-weight:400}
</style></head><body><main>
<h1>Connect your Dino</h1>
<p>So it can show the time. Dino only uses Wi-Fi to ask for the time, about once a day.</p>
<label for="n">Your Wi-Fi network</label>
<select id="n"><option value="">Looking for networks...</option></select>
<button class="alt" id="r" type="button">Look again</button>
<label for="m">Or type its name</label><input id="m" type="text" autocomplete="off" autocapitalize="off" maxlength="32">
<label for="p">Wi-Fi password</label><input id="p" type="password" autocomplete="off" maxlength="63">
<label class="row"><input type="checkbox" id="s"> Show password</label>
<button id="go" type="button">Connect</button>
<p id="msg" role="status"></p>
<small>Dino can only use 2.4 GHz networks. Your password goes to dino only, over its own network, and is kept on dino.</small>
<script>
const $=i=>document.getElementById(i);let busy=false;
const say=t=>{$('msg').textContent=t};
async function scan(){
  try{const r=await(await fetch('/scan')).json();
    if(!r.done){setTimeout(scan,1000);return}
    const n=$('n');n.textContent='';
    r.list.forEach(x=>{const o=document.createElement('option');o.value=x.s;o.textContent=x.s+(x.o?'':' (open)');n.appendChild(o)});
    if(!r.list.length){const o=document.createElement('option');o.value='';o.textContent='No networks found';n.appendChild(o)}
  }catch(e){setTimeout(scan,2000)}
}
$('r').onclick=async()=>{$('n').textContent='';await fetch('/scan?again=1');scan()};
$('s').onchange=()=>{$('p').type=$('s').checked?'text':'password'};
async function poll(){
  try{const r=await(await fetch('/status')).json();
    if(r.state==='done'){say('Connected! Dino is getting the time. You can close this page.');return}
    if(r.state==='failed'){busy=false;$('go').disabled=false;say('Dino could not connect. Check the password and that it is a 2.4 GHz network, then try again.');return}
  }catch(e){}
  setTimeout(poll,1000);
}
$('go').onclick=async()=>{
  if(busy)return;
  const ssid=$('m').value||$('n').value;
  if(!ssid){say('Pick your Wi-Fi network first.');return}
  busy=true;$('go').disabled=true;say('Connecting...');
  const body=new URLSearchParams({ssid:ssid,pass:$('p').value}).toString();
  try{const r=await(await fetch('/save',{method:'POST',headers:{'Content-Type':'text/plain'},body:body})).json();
    if(!r.ok){busy=false;$('go').disabled=false;say(r.msg);return}
    poll();
  }catch(e){busy=false;$('go').disabled=false;say('Lost dino. Check that your phone is still on the DINO network.')}
};
scan();
</script></main></body></html>)HTML";

static const IPAddress AP_IP(192, 168, 4, 1);

bool WifiPortal::start(const char* apName, const char* apPassword, Session& session) {
  stop();
  session_ = &session;
  WiFi.mode(WIFI_AP_STA);  // the station side is for trying the home Wi-Fi and for scanning
  WiFi.softAPConfig(AP_IP, AP_IP, IPAddress(255, 255, 255, 0));
  if (!WiFi.softAP(apName, apPassword, 1, 0, WIFI_SETUP_MAX_PHONES)) {
    Serial.println("[wifi] could not start the setup network");
    WiFi.mode(WIFI_OFF);
    return false;
  }
#if WIFI_LOW_TX_POWER
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
#endif
  dns_.setErrorReplyCode(DNSReplyCode::NoError);
  dns_.start(53, "*", AP_IP);  // every name -> dino: phones notice the "sign in" page and open it
  routes();
  web_.begin();
  scanning_ = false;
  scanDoneMs_ = 0;
  startScan();  // look for networks now, while no phone is on the network yet (a scan makes the radio hop channels)
  lastState_ = session.state();
  running_ = true;
  Serial.printf("[wifi] setup network \"%s\" is up\n", apName);
  return true;
}

void WifiPortal::stop() {
  if (!running_) return;
  web_.stop();
  dns_.stop();
  WiFi.scanDelete();
  WiFi.softAPdisconnect(true);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  running_ = false;
  Serial.println("[wifi] setup network is down");
}

void WifiPortal::routes() {
  web_.on("/", HTTP_GET, [this]() { sendPage(); });
  web_.on("/scan", HTTP_GET, [this]() { sendScan(); });
  web_.on("/save", HTTP_POST, [this]() { sendSave(); });
  web_.on("/status", HTTP_GET, [this]() { sendStatus(); });
  web_.onNotFound([this]() { redirectHome(); });  // /generate_204, /hotspot-detect.html, /connecttest.txt ...: the "sign in" probes
}

void WifiPortal::reply(int code, const char* json) {
  web_.sendHeader("Cache-Control", "no-store");
  web_.send(code, "application/json", json);
}

void WifiPortal::sendPage() {
  web_.sendHeader("Cache-Control", "no-store");
  web_.send_P(200, "text/html", PAGE);
}

void WifiPortal::redirectHome() {
  web_.sendHeader("Location", "http://192.168.4.1/", true);
  web_.send(302, "text/plain", "");
}

void WifiPortal::startScan() {
  if (scanning_) return;
  WiFi.scanDelete();
  WiFi.scanNetworks(true);  // async: the loop keeps running
  scanning_ = true;
}

// {"done":false} while scanning, then {"done":true,"list":[{"s":"name","o":1},...]} (o = 1 when it has a password).
void WifiPortal::sendScan() {
  if (web_.hasArg("again")) {
    startScan();
    reply(200, "{\"done\":false}");
    return;
  }
  if (scanning_) { reply(200, "{\"done\":false}"); return; }
  const int n = WiFi.scanComplete();
  if (n < 0) { startScan(); reply(200, "{\"done\":false}"); return; }
  String out = "{\"done\":true,\"list\":[";
  int listed = 0;
  String seen[WIFI_SETUP_SCAN_MAX];
  for (int i = 0; i < n && listed < WIFI_SETUP_SCAN_MAX; i++) {
    const String name = WiFi.SSID(i);
    if (!name.length() || name.length() > wifisetup::SSID_MAX) continue;  // hidden or unusable
    bool dup = false;
    for (int k = 0; k < listed; k++) if (seen[k] == name) { dup = true; break; }
    if (dup) continue;
    char esc[2 * wifisetup::SSID_MAX + 8];
    if (!wifisetup::jsonString(name.c_str(), esc, sizeof esc)) continue;
    if (listed) out += ',';
    out += "{\"s\":";
    out += esc;
    out += WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? ",\"o\":0}" : ",\"o\":1}";
    seen[listed++] = name;
  }
  out += "]}";
  reply(200, out.c_str());
}

// POST text/plain "ssid=..&pass=..". The session decides whether it is usable; if it is, dino starts joining.
void WifiPortal::sendSave() {
  const String body = web_.arg("plain");
  char ssid[wifisetup::SSID_MAX + 2] = "", pass[wifisetup::PASSPHRASE_MAX + 2] = "";
  // formValue is false when a field is missing or does not fit in the buffer (one byte longer than allowed), so a
  // too-long password is refused here and never turns into an empty one (= an open network).
  Check c = Check::BadChars;
  if (body.length() > 0 && body.length() < 400) {
    if (!wifisetup::formValue(body.c_str(), "ssid", ssid, sizeof ssid)) c = Check::SsidTooLong;
    else if (!wifisetup::formValue(body.c_str(), "pass", pass, sizeof pass)) c = Check::PassTooLong;
    else c = session_->submit(ssid, pass, millis());
  }
  if (c != Check::Ok) {
    String out = "{\"ok\":false,\"msg\":\"";
    out += wifisetup::checkMessage(c);
    out += "\"}";
    reply(200, out.c_str());
    return;
  }
  Serial.printf("[wifi] trying \"%s\"\n", session_->ssid());  // the password is never printed
  WiFi.scanDelete();
  scanning_ = false;
  WiFi.begin(session_->ssid(), session_->pass());
  reply(200, "{\"ok\":true}");
}

void WifiPortal::sendStatus() {
  const char* st = "waiting";
  switch (session_->state()) {
    case Session::State::Connecting: st = "connecting"; break;
    case Session::State::Done: st = "done"; break;
    default: st = session_->lastFailed() ? "failed" : "waiting"; break;
  }
  String out = "{\"state\":\"";
  out += st;
  out += "\"}";
  reply(200, out.c_str());
}

void WifiPortal::loop(uint32_t now) {
  if (!running_) return;
  dns_.processNextRequest();
  web_.handleClient();
  if (scanning_ && WiFi.scanComplete() != WIFI_SCAN_RUNNING) { scanning_ = false; scanDoneMs_ = now; }

  Session& s = *session_;
  if (s.state() != Session::State::Connecting) s.onStations(WiFi.softAPgetStationNum(), now);
  if (s.state() == Session::State::Connecting) {
    const wl_status_t w = WiFi.status();
    if (w == WL_CONNECTED) s.onConnectResult(true, now);
    else if (w == WL_CONNECT_FAILED || w == WL_NO_SSID_AVAIL) s.onConnectResult(false, now);  // refused / not found: no need to wait
  }
  s.tick(now);
  if (lastState_ == Session::State::Connecting && s.state() != Session::State::Connecting &&
      s.state() != Session::State::Done) {
    WiFi.disconnect(false);  // gave up on this try: stop retrying in the background (the AP stays up)
    Serial.println("[wifi] could not join that network");
  }
  lastState_ = s.state();
}
