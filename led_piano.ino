/*
 * Kawai CA63  ->  USB-MIDI-Host (ESP32-S3)  ->  reaktives Ambiente-Licht (FastLED)
 * --------------------------------------------------------------------------------
 * EIN Geraet: liest per USB-MIDI, was gespielt wird, und treibt einen WS2812B-Streifen
 * als Stimmungslicht. Kein Mikro, kein PC, kein WLAN.
 *
 * Reaktion:
 *   - jeder Anschlag fuettert "Energie" (nach Velocity) -> Helligkeit/Lebendigkeit
 *   - Tonlage steuert die Grundfarbe (Bass = warm/rot, Hoehen = kuehl/blau)
 *   - Sustain-Pedal (CC64) laesst die Energie langsamer abklingen (laengeres Nachgluehen)
 *   - im Ruhezustand: ruhiges, gedimmtes Atmen
 *
 * BENOETIGT:
 *   - Board: ESP32-S3 (USB-Host-faehig, z.B. ESP32-S3-USB-OTG)
 *   - Arduino: arduino-esp32 Core >= 3.0.0 ; Tools > USB Mode > "USB Host"
 *   - Libraries: ESP32_Host_MIDI (sauloverissimo) , FastLED
 *
 * Die mit >>> MIDI-LIB <<< markierten Zeilen folgen der ESP32_Host_MIDI-README;
 * falls dein examples/-Sketch abweicht, uebernimm sie 1:1. Der Rest ist davon unabhaengig.
 */

#include <FastLED.h>
// >>> MIDI-LIB <<< (1/4)
#include <ESP32_Host_MIDI.h>
#include <USBConnection.h>

// ===================== KONFIG: HIER ANPASSEN =====================
#define LED_PIN      21         // freier GPIO fuer Daten (Board-Pinout pruefen!)
#define NUM_LEDS     90         // an Streifenlaenge anpassen (60 LED/m * 1,5 m = 90)
#define LED_TYPE     WS2812B
#define COLOR_ORDER  GRB
#define MAX_BRIGHT   180        // 0..255 Obergrenze (schont Netzteil/Augen)

// ESP32-S3-USB-OTG: 5V VBUS der USB-A-Host-Buchse einschalten (sonst meldet sich das Klavier nicht)
#define BOARD_ESP32S3_USB_OTG  1
// ================================================================

CRGB leds[NUM_LEDS];

// >>> MIDI-LIB <<< (2/4)
USBConnection usbHost;
MidiHandler   midiHandler;

// --- reaktiver Zustand ---
float    energy     = 0.0f;     // 0..1
float    hueTarget  = 110.0f;   // Ziel-Farbton (nach Tonlage)
float    hue        = 110.0f;   // geglaetteter Farbton
bool     sustain    = false;
uint32_t lastFrame  = 0;

void onNote(uint8_t note, uint8_t vel) {
  energy += (vel / 127.0f) * 0.55f;
  if (energy > 1.0f) energy = 1.0f;
  // Tonlage A0(21)..C8(108) -> Farbton 0 (rot/warm) .. 175 (blau/kuehl)
  float t = (note - 21) / 87.0f; if (t < 0) t = 0; if (t > 1) t = 1;
  hueTarget = t * 175.0f;
}

void processMidi(uint8_t status, uint8_t d1, uint8_t d2) {
  uint8_t hi = status & 0xF0;
  if (hi == 0x90 && d2 > 0)            onNote(d1, d2);          // Note On
  else if (hi == 0xB0 && d1 == 64)     sustain = (d2 >= 64);    // Sustain-Pedal
}

void setup() {
#if BOARD_ESP32S3_USB_OTG
  pinMode(18, OUTPUT); digitalWrite(18, HIGH);  // USB_SEL  -> Host
  pinMode(17, OUTPUT); digitalWrite(17, HIGH);  // LIMIT_EN
  pinMode(13, OUTPUT); digitalWrite(13, LOW);   // BOOST_EN aus
  pinMode(12, OUTPUT); digitalWrite(12, HIGH);  // DEV_VBUS_EN -> 5V an die Klavier-Buchse
  delay(50);
#endif

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(MAX_BRIGHT);

  // >>> MIDI-LIB <<< (3/4)
  midiHandler.addTransport(&usbHost);
  usbHost.begin();
  midiHandler.begin();
}

void loop() {
  // >>> MIDI-LIB <<< (4/4): eingehende Events abholen
  midiHandler.task();
  for (const auto& ev : midiHandler.getQueue()) {
    uint8_t status = ev.statusCode | ev.channel0;
    processMidi(status, ev.noteNumber, ev.velocity7);
  }

  uint32_t now = millis();
  if (now - lastFrame >= 16) {                 // ~60 fps
    float dt = (now - lastFrame) / 1000.0f;
    lastFrame = now;

    // Energie abklingen (Pedal -> langsamer)
    float decay = sustain ? 0.45f : 1.0f;      // Anteil pro Sekunde
    energy -= energy * decay * dt;
    if (energy < 0) energy = 0;

    // Farbton sanft nachziehen
    hue += (hueTarget - hue) * (dt * 2.5f > 1 ? 1 : dt * 2.5f);

    // Grund-Glow + Energie -> Gesamthelligkeit
    float bri = 0.12f + energy * 0.88f;        // 0.12 .. 1.0
    uint8_t v = (uint8_t)(bri * 255);

    // sanftes, fliessendes Feld
    for (int i = 0; i < NUM_LEDS; i++) {
      uint8_t wave = sin8((uint8_t)(i * 6) + (uint8_t)(now / 24));   // langsame Welle
      uint8_t h    = (uint8_t)hue + (wave >> 4);                      // leichte Farbvariation
      uint8_t vv   = scale8(v, 175 + (wave >> 2));                    // Helligkeit moduliert
      leds[i] = CHSV(h, 230, vv);
    }
    FastLED.show();
  }
}
