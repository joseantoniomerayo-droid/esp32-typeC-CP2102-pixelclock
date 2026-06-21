#include <Arduino.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "display.h"
#include "nvs_config.h"
#include "ble_config.h"
#include "clock.h"

#define PANEL_BRIGHTNESS 40

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== RELOJ + CLIMA HUB75 ===");

  // NVS persistente
  nvsInit();

  // BLE + WiFi (intenta conectar si hay credenciales en NVS)
  initBLE();

  // Display
  initDisplay();
  dma_display->setPanelBrightness(10);
  delay(1000);
  dma_display->setPanelBrightness(PANEL_BRIGHTNESS);

  // NTP
  showConnecting("NTP...");
  syncNTP();

  Serial.println("Ready");
}

void loop() {
  drawClock();
  delay(10);
}
