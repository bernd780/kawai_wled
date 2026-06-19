# kawai_wled

**Reaktives Ambiente‑Licht für ein Kawai‑Digitalpiano — direkt per USB‑MIDI.**

Ein **ESP32‑S3** hängt am USB‑„To Host"‑Port des Klaviers, liest als **USB‑MIDI‑Host** die gespielten
Noten und treibt einen **WS2812B/SK6812‑Streifen** als Stimmungslicht. **Kein Mikrofon, kein PC, kein WLAN.**

> Hinweis zum Namen: Stock‑**WLED** kann **kein USB‑MIDI** lesen (es reagiert nur per Mikrofon über den
> AudioReactive‑Usermod). Diese kleine, eigenständige Firmware liest das Klavier **direkt** und macht
> daraus das gewünschte „WLED‑artige" Ambiente.

## Wie es reagiert
- **Anschlagstärke (Velocity)** → Energie → Helligkeit/Lebendigkeit (klingt sanft ab)
- **Tonlage** → Grundfarbe: Bass = warm/rot, Höhen = kühl/blau
- **Sustain‑Pedal (CC64)** → längeres Nachglühen
- **Ruhe** → ruhiges, gedimmtes „Atmen"

Alle Effekt‑Parameter stehen oben im Sketch (`energy +=`, `decay`, Farb­spanne, `MAX_BRIGHT`).

## Hardware
| Teil | Empfehlung |
|---|---|
| Controller | **ESP32‑S3** (USB‑Host‑fähig) |
| LEDs | **WS2812B 60 LED/m** (oder SK6812 RGBW) |
| Netzteil | **5 V**, nach Streifenlänge (lange Streifen beidseitig einspeisen) |
| Kleinkram | Level‑Shifter (74AHCT125) oder ~330 Ω am Daten‑Pin, 1000 µF am 5 V |

### USB anschließen — zwei Wege
**A) ESP32‑S3‑USB‑OTG‑Board (stecken & fertig):** hat USB‑A‑Host‑Buchse + schaltbare 5 V VBUS.
Im Sketch `BOARD_ESP32S3_USB_OTG 1` lassen.

**B) Beliebiges ESP32‑S3‑DevKit selbst verdrahten** (günstiger, mehr freie GPIOs):
USB‑A‑Buchse anlöten und im Sketch `BOARD_ESP32S3_USB_OTG 0` setzen.

| USB‑A‑Buchse (zum Klavier per A‑B‑Kabel) | an |
|---|---|
| **VBUS (+5 V)** | **+5 V vom Netzteil** (der S3 liefert selbst kein VBUS!) |
| **D−** | GPIO19 |
| **D+** | GPIO20 |
| **GND** | gemeinsame Masse (ESP32 + Netzteil) |

LED‑Daten an `LED_PIN` (Default 21 — freien Pin laut Board‑Pinout wählen), gemeinsame Masse.

## Build (Arduino IDE)
1. Boardpaket **„esp32 by Espressif" ≥ 3.0.0**, Board: **ESP32S3 Dev Module**.
2. **Tools → USB Mode → „USB Host"**.
3. Libraries: **ESP32_Host_MIDI** (sauloverissimo), **FastLED**.
4. In `led_piano.ino`: `NUM_LEDS`, `LED_PIN`, `BOARD_ESP32S3_USB_OTG`, ggf. `MAX_BRIGHT` anpassen.
5. Hochladen → Klavier anschließen → spielen.

## Status
Funktionsfähige Erst‑Firmware. Getestet mit einem Kawai CA63 (USB‑MIDI, Kanal 1, Velocity, CC64).
Beiträge/Ideen willkommen.
