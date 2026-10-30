# Ground rules (dino)

| Area | Decision |
|---|---|
| Hardware | ESP32-C3, 0.96" SSD1306, MPU6050, TTP223, piezo. No hardware changes. |
| Code | One shared firmware base (ADR 003). Add, don't rewrite. |
| Independence | Standalone project and repo with its own phase and rules. |
| Device | Complete on its own. No camera, mic or cloud. |
| Tone | No guilt, no nagging, no strobing (only the 20x2 px set-alarm underline blinks, 1.25 Hz). |
| Utility | Exact. Alarms and timers never wait for a mood or animation. |
| Scope | v1 = R, L, U. Games and Friends need their own ADR (Friends needs a radio and a privacy check). |
| Ownership | Original IP; the face is the sheet's design. |

Every feature must express personality, reveal something, or build continuity; otherwise cut it.
