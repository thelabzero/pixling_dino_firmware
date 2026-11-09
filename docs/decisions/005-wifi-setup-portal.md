# 005 · Wi-Fi setup: "Connect your dino" (SoftAP captive portal)
**Status:** accepted (owner), 2026-11-07. Code complete and tested on the desktop; not yet run on the board (`docs/wifi-setup-checklist.md`).

## Problem
Dino needs Wi-Fi only to ask a time server (ADR 010: at start-up and once a day, radio off in between). The network name and password were compiled in from `include/secrets.h`, so every change meant a rebuild. Dino has no keyboard and a 128x64 screen: the password has to be typed on a phone.

## Decision
Menu > **Wi-Fi** (new 5th card, Back moves to 6th) opens "connect your dino":
1. Dino starts its own temporary network `DINO-1234` (WPA2) and shows its name and a 10-character password on the OLED, with the message "CONNECT YOUR DINO TO SHOW THE TIME" and the dino looking around while the Wi-Fi logo's arcs light up.
2. A phone joins it; a DNS server answers every name with dino, so the phone opens the setup page by itself (captive portal). The page lists the networks dino can see, takes the password, and shows progress.
3. Dino tries the home Wi-Fi (up to 20 s). On success: happy clip, credentials saved to flash (NVS), the setup network goes down 4 s later, and the time syncs. On failure: sad clip, back to the pairing screen, try again.
4. Long press leaves at any time. No phone within 5 minutes: gives up. An alarm always wins (it closes the setup).

Pairing rules (credential checks, form decoding, the code, the session state machine and its time-outs) are pure C++ in `firmware/src/wifi/` with desktop tests. The radio side is `drivers/wifi_portal.*`; the owner of the radio is the existing `WifiClock` module (so modules still never call each other). `secrets.h` stays as a developer fallback when nothing is saved.

## Why not BLE (considered)
Web Bluetooth does not exist in iPhone Safari, the page would need to be hosted on HTTPS somewhere, and NimBLE costs about 100-150 KB of flash (estimate). A SoftAP page works on every phone, needs no app and no hosting. Web Serial (USB) stays a developer tool (like the IMU calibrator), not the user path.

## Security (facts and assumptions)
- The setup network is **WPA2**, not open. An open network would carry the home Wi-Fi password in clear to anyone listening nearby.
- The network password is 10 characters from a 32-character alphabet without look-alikes (50 bits), random per setup, shown only on dino's screen: whoever sets it up has to be looking at the device. An 8-digit code (27 bits) was rejected: assumption, not measured: a recorded handshake could be cracked offline in minutes on a GPU.
- The code comes from the hardware random generator with the radio switched on first (with the radio off, `esp_random()` is only pseudo-random).
- Credentials are stored **as plain text** in NVS: the chip has no flash encryption here. Someone with the device in hand and a USB cable can read them. Acceptable for a desk toy; do not reuse a sensitive password. They are written only after a successful connect (flash wear). They are never printed to the serial log.
- The page is plain HTTP on dino's own network (no internet, no outside resources). Network names from the scan are put into the page as text, never as HTML (checked in a browser against a hostile name).
- Window: at most 5 minutes, at most 2 phones.

## Known limits
- **2.4 GHz only** (the ESP32-C3 has no 5 GHz). The page and the failure screen say so.
- **Channel switch:** when dino joins the home router, its own network moves to the router's channel. The phone may lose dino for a moment and the page may not show "Connected". The dino screen is the source of truth. (Assumption from how the ESP32 soft AP behaves; check on the board.)
- **Scan:** done once when the setup opens (async, no blocking) and again when "Look again" is pressed; a scan makes the radio hop channels for a moment.
- **No extra feature switch:** the menu always has the Wi-Fi card; with `FEATURE_WIFI_TIME` off the pairing screen would wait for events that never come (long press leaves).
- **Hidden networks** can be typed by name on the page; open networks work (empty password).
- Flash: +6 points (69.6% -> 75.6%, measured) for the web server, DNS and soft AP.

## Owner design (the sheet has no Wi-Fi screens)
Like the stopwatch icon, the Wi-Fi card icon and the clips WifiPair / WifiDone / WifiFail are drawn in code (`scenes/dino_live.cpp`) in the sheet's style (small face at x24 y31, text beside it, 1 px lines). They are not in `art/reference/dino-v1/`, so `face_check.py` does not cover them; `tools/live_native.cpp` renders every one. Add them to the sheet when it is next revised.
Sounds reuse existing ones: connected = `alarm_set` (happy warble), failed = `snooze` (grumpy "hmph"). A dedicated pair can replace them.

## Setup page look (2026-11-08)
The page uses the Pixling brand language, like the boot film (ADR 006): the pixel "Pixling" wordmark with its teal hopping dot, the pixel Dino beside the title (its face changes: open while waiting, happy on "Connected", unimpressed on a failure), Night `#282020` / cream `#FCF8F0` / teal `#2F9083`, 4 px strokes, stepped corners (clip-path), no curves, light and dark by the phone's setting, reduced motion respected. The art is generated from the same bitmaps as the boot film (`tools/gen_portal_art.py`), so the logo cannot drift. Still no outside resources: fonts fall back to the phone's own (the sheet's Atkinson Hyperlegible is used only if the phone has it; embedding it would cost about 25 KB of flash per weight, not done). Facts: the page JavaScript and element ids are unchanged; it was re-run in headless Chrome against the route mock (success, wrong password, rejected input, hostile network name): same results as before; 8.2 KB.

## Not done (next)
- Time board with no time: could say "CONNECT YOUR DINO / MENU > WI-FI" when nothing is saved (needs the brain to know whether credentials exist).
- Forget the saved network (menu item or key), set timezone on the same page.
