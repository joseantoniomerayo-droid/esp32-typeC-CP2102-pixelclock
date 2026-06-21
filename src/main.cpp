#include <Arduino.h>
#include <WiFiManager.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "display.h"
#include "nvs_config.h"
#include "clock.h"
#include "mqtt_handler.h"
#include "qr_display.h"

#define PANEL_BRIGHTNESS 40
#define PIN_BOOT         0    // BOOT button en DMDos V3 (pull-up)
#define RESET_HOLD_MS    3000 // ms para resetear WiFi manteniendo BOOT

// ─── Reset WiFi si se pulsa BOOT al arrancar ──────────
static void checkHardReset() {
  pinMode(PIN_BOOT, INPUT_PULLUP);
  if (digitalRead(PIN_BOOT) == LOW) {
    Serial.println("BOOT pulsado — limpiando config WiFi...");
    WiFiManager wm;
    wm.resetSettings();
    nvsSaveStr("wifi_ssid", "");
    nvsSaveStr("wifi_pass", "");
    Serial.println("Config borrada. Reiniciando...");
    delay(1000);
    ESP.restart();
  }
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);
  delay(500);

  nvsInit();

  // Reset WiFi si GPIO0 pulsado al arrancar
  checkHardReset();

  // Display
  initDisplay();
  dma_display->setPanelBrightness(PANEL_BRIGHTNESS);

  // WiFiManager
  WiFiManager wm;
  wm.setConfigPortalTimeout(180);
  wm.setTitle("PixelClock");
  wm.setConnectTimeout(15);

  showQRCode();

  if (!wm.autoConnect("PixelClock-AP")) {
    Serial.println("WiFi timeout — reiniciando");
    ESP.restart();
  }

  showConnecting("Conectando...");
  dma_display->setPanelBrightness(10);
  delay(500);

  // MQTT
  showConnecting("MQTT...");
  initMQTT();

  // NTP
  showConnecting("NTP...");
  syncNTP();

  dma_display->setPanelBrightness(PANEL_BRIGHTNESS);
  Serial.println("Ready");
}

void loop() {
  mqttLoop();
  drawClock();
  delay(10);
}
