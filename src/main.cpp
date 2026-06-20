#include <Arduino.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "display.h"
#include "clock.h"

#define PANEL_BRIGHTNESS 40

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== RELOJ + CLIMA HUB75 ===");

  initDisplay();
  dma_display->setPanelBrightness(10);
  delay(1000);
  dma_display->setPanelBrightness(PANEL_BRIGHTNESS);

  showConnecting("WiFi...");
  initWiFi();

  showConnecting("NTP...");
  syncNTP();

  Serial.println("Ready");
}

void loop() {
  drawClock();
  delay(10);
}
