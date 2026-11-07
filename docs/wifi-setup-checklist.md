# Wi-Fi setup board checklist (dino_c3)

Run after flashing `dino_c3`. Use a phone (and, if you can, a second one of the other kind: iPhone and Android). Serial monitor open (`-f time`). Fill the result column and paste any failing log.

```
cd firmware
.venv/bin/pio run -e dino_c3 -t upload && .venv/bin/pio device monitor -e dino_c3 -f time
```

To test from nothing: type `i` first (shows whether a network is saved). The credentials survive a normal upload; to start clean temporarily rename `secrets.h` and erase flash once with `.venv/bin/pio run -e dino_c3 -t erase`.

| # | Check | Pass when | Result |
|---|---|---|---|
| 1 | Menu card | Long press, tilt: a Wi-Fi card (logo) sits between Stopwatch and Back; six dots under the card | |
| 2 | Pairing screen | Tap on it: "CONNECT YOUR / DINO TO SHOW / THE TIME" with the logo's arcs lighting one by one and the dino's eyes sweeping; then a page with `DINO-xxxx` and a password in two groups of 5; loops. Log: `[wifi] setup network "DINO-xxxx" is up` | |
| 3 | Join | Phone Wi-Fi list shows `DINO-xxxx`; join with the password from the screen. Screen changes to "FINISH ON YOUR PHONE" | |
| 4 | Page opens | The phone opens "Connect your Dino" by itself (if not, browse to `192.168.4.1`). iPhone: the "sign in" sheet. Android: "sign in to network" notification | |
| 5 | Network list | After a few seconds your networks are listed; "Look again" refreshes; hidden or typed names work | |
| 6 | Wrong password | Connect with a wrong password: within about 20 s the page says it could not connect, the dino shows the sad "NO LUCK / CHECK THE PASSWORD", then the pairing screen returns. Log shows `could not join that network` and no password | |
| 7 | Right password | Connect: screen "CONNECTING", then happy "CONNECTED" with a heart, then the time (or "GETTING THE TIME" and then the time). Log: `connected to "..."; saved`, then `[time] synced` | |
| 8 | Network goes down | About 4 s after success the DINO network disappears from the phone's list. The phone may have dropped it earlier (channel switch): the dino screen is what counts | |
| 9 | Survives reset | Press reset: time syncs by itself with no `no Wi-Fi saved` message | |
| 10 | Cancel | Open Wi-Fi again, long press: back home at once, network gone | |
| 11 | Time-out | Open Wi-Fi and do nothing for 5 minutes: back home by itself, network gone | |
| 12 | Alarm wins | Set an alarm for the next minute, open Wi-Fi: the alarm rings on time and the network goes down | |
| 13 | Time sync during setup | While pairing is open the daily sync does not run (no `[time] connecting` lines until it closes) | |
| 14 | Memory | After several setups in a row: `i` shows heap near the usual 215 KB and stack left near 9 KB | |
| 15 | Both phone kinds | 2-6 behave the same on iPhone and Android | |
