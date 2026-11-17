# ADR 008: The user guide page follows the Pixling brand language

Status: accepted (owner request), 2026-11-09. Source: "Pixling Brand Language" v1 (November 2026).

## Decision
The guide at https://thelabzero.github.io/pixling-dino-guide/ (repo `pixling-dino-guide`, separate from this one) uses the brand sheet's rules, like the boot film (ADR 006) and the Wi-Fi setup page (ADR 005):
- Colour: Night `#282020`, Pixel cream `#FCF8F0`, Teal spark `#2F9083` as one small thing per view (section numbers, the logo's dot, focus ring, hover). Ice blue `#7FE7FF` only on the device-screen animations (on `#0A0808`). Light mode (cream ground, darker teal `#1F7A6E` for text) follows the visitor's setting; the device screens stay dark in both.
- Type: IBM Plex Sans and Mono as in the sheet, self-hosted (latin subset, about 60 KB), no outside requests.
- Mark: the "Pixling" wordmark with its hopping teal dot and Dino (blinking), as inline SVG from the same bitmaps as the boot film (`gen_brand_art.py`), so they cannot drift. Whole cells, no curves; steps and tags use stepped corners or squares.
- Layout: numbered mono section labels as in the sheet; cards `#1E1818`; no extra accent colour (the old amber warning became a cream bar).
- Motion: whole steps, held; off with reduced motion.

## Not done
- The IMU calibrator page (developer tool) keeps its own blue/green palette.
- Light mode and phone width were only reasoned about, not looked at (desktop dark was screenshotted).
- Sheet voice (lowercase, short) applies to the device, not to the guide's prose; left as is.
