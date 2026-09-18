#include <Arduino.h>

#ifndef ARDUINO_USB_MODE
#error "Valitse Tools > USB Mode: USB-OTG (TinyUSB)"
#elif ARDUINO_USB_MODE == 1
#error "Valitse Tools > USB Mode: USB-OTG (TinyUSB), ei Hardware CDC"
#endif

#include "USB.h"
#include "USBHIDGamepad.h"
#include <TFT_eSPI.h>

USBHIDGamepad Gamepad;
TFT_eSPI tft = TFT_eSPI();

#define BTN1_PIN 1
#define BTN2_PIN 2
#define BTN3_PIN 11
#define BTN4_PIN 12
#define LED_PIN  13
#define PIN_POWER_ON 15

const uint8_t BTN_PINS[4] = {BTN1_PIN, BTN2_PIN, BTN3_PIN, BTN4_PIN};
bool btnState[4] = {false, false, false, false};
bool btnLast[4]  = {false, false, false, false};
unsigned long btnLastChange[4] = {0, 0, 0, 0};
const unsigned long DEBOUNCE_MS = 15;

struct Telemetry {
  int   rpm      = 0;
  int   speed    = 0;
  String gear    = "-";
  String lapTime = "--:--.---";
  float delta    = 0.0;
} tele;

// Edelliset piirretyt arvot - vilkkumisen estoon
int   lastRpm   = -1;
int   lastSpeed = -1;
String lastGear = "";
String lastLap  = "";
float lastDelta = -9999;

String rxLine = "";
unsigned long lastDataReceived = 0;
const unsigned long DATA_TIMEOUT_MS = 2000;

void drawStaticLabels() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(TL_DATUM);

  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.setTextFont(2);
  tft.drawString("RPM", 10, 4);
  tft.drawString("KM/H", 10, 54);
  tft.drawString("VAIHDE", 230, 4);
  tft.drawString("KIERROSAIKA", 10, 104);
  tft.drawString("DELTA", 170, 104);
}

void drawTelemetry() {
  if (tele.rpm != lastRpm) {
    tft.fillRect(10, 22, 110, 26, TFT_BLACK);
    tft.setTextFont(4);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString(String(tele.rpm), 10, 22);
    lastRpm = tele.rpm;
  }

  if (tele.speed != lastSpeed) {
    tft.fillRect(10, 72, 90, 26, TFT_BLACK);
    tft.setTextFont(4);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(String(tele.speed), 10, 72);
    lastSpeed = tele.speed;
  }

  if (tele.gear != lastGear) {
    tft.fillRect(225, 20, 70, 80, TFT_BLACK);
    tft.setTextSize(3);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString(tele.gear, 230, 22);
    tft.setTextSize(1);
    lastGear = tele.gear;
  }

  if (tele.lapTime != lastLap) {
    tft.fillRect(10, 122, 150, 16, TFT_BLACK);
    tft.setTextFont(2);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString(tele.lapTime, 10, 122);
    lastLap = tele.lapTime;
  }

  if (abs(tele.delta - lastDelta) > 0.001) {
    tft.fillRect(170, 122, 90, 16, TFT_BLACK);
    tft.setTextFont(2);
    tft.setTextColor(tele.delta <= 0 ? TFT_GREEN : TFT_RED, TFT_BLACK);
    String deltaStr = (tele.delta >= 0 ? "+" : "") + String(tele.delta, 3);
    tft.drawString(deltaStr, 170, 122);
    lastDelta = tele.delta;
  }
}

void parseTelemetry(const String &line) {
  int start = 0;
  while (start < (int)line.length()) {
    int semi = line.indexOf(';', start);
    if (semi == -1) semi = line.length();
    String field = line.substring(start, semi);
    int colon = field.indexOf(':');
    if (colon > 0) {
      String key = field.substring(0, colon);
      String val = field.substring(colon + 1);
      if      (key == "R") tele.rpm   = val.toInt();
      else if (key == "S") tele.speed = val.toInt();
      else if (key == "G") tele.gear  = val;
      else if (key == "L") tele.lapTime = val;
      else if (key == "D") tele.delta = val.toFloat();
    }
    start = semi + 1;
  }
  drawTelemetry();
  lastDataReceived = millis();
}

void setup() {
  pinMode(PIN_POWER_ON, OUTPUT);
  digitalWrite(PIN_POWER_ON, HIGH);

  for (uint8_t i = 0; i < 4; i++) {
    pinMode(BTN_PINS[i], INPUT_PULLUP);
  }
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Gamepad.begin();
  USB.begin();

  Serial.begin(115200);

  tft.init();
  tft.setRotation(1);
  drawStaticLabels();
}

void loop() {
  unsigned long now = millis();
  for (uint8_t i = 0; i < 4; i++) {
    bool pressed = (digitalRead(BTN_PINS[i]) == LOW);
    if (pressed != btnLast[i]) {
      btnLastChange[i] = now;
      btnLast[i] = pressed;
    }
    if ((now - btnLastChange[i]) > DEBOUNCE_MS && pressed != btnState[i]) {
      btnState[i] = pressed;
      if (pressed) {
        Gamepad.pressButton(i);
      } else {
        Gamepad.releaseButton(i);
      }
    }
  }

  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      rxLine.trim();
      if (rxLine.length() > 0) parseTelemetry(rxLine);
      rxLine = "";
    } else if (c != '\r') {
      rxLine += c;
    }
  }

  digitalWrite(LED_PIN, (now - lastDataReceived) < DATA_TIMEOUT_MS ? HIGH : LOW);
}
