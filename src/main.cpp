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

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== RELOJ + CLIMA HUB75 ===");

  // NVS persistente
  nvsInit();

  // Display
  initDisplay();
  dma_display->setPanelBrightness(PANEL_BRIGHTNESS);

  // WiFiManager — portal cautivo si no hay credenciales
  WiFiManager wm;
  wm.setConfigPortalTimeout(180);
  wm.setTitle("PixelClock");

  // Mostrar QR en el panel mientras espera configuracion
  showQRCode();

  if (!wm.autoConnect("PixelClock-AP")) {
    Serial.println("WiFi: timeout, reiniciando...");
    ESP.restart();
  }
  dma_display->setPanelBrightness(10);
  showConnecting("Conectando...");
  delay(500);

  Serial.printf("WiFi: %s (%s)\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());

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
