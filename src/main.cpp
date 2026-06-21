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
#define PIN_BOOT         0

// ─── Reset WiFi manteniendo BOOT 5s durante operacion ──
static void checkFactoryReset() {
  pinMode(PIN_BOOT, INPUT_PULLUP);
  delay(50);
  if (digitalRead(PIN_BOOT) == LOW) {
    unsigned long t = millis();
    // Mostrar cuenta atras en panel
    dma_display->fillScreen(0);
    dma_display->setTextSize(1);
    dma_display->setTextColor(dma_display->color565(255,200,0));
    dma_display->setCursor(10, 8);
    dma_display->print("BOOT pulsado");
    dma_display->setCursor(10, 18);
    dma_display->print("Manten 5s para reset");
    dma_display->flipDMABuffer();

    while (digitalRead(PIN_BOOT) == LOW && millis() - t < 5000) {
      delay(100);
    }

    if (digitalRead(PIN_BOOT) == HIGH) {
      return;  // solto antes de tiempo
    }

    // 5s alcanzados, resetear
    dma_display->fillScreen(0);
    dma_display->setCursor(10, 12);
    dma_display->setTextColor(dma_display->color565(255,80,80));
    dma_display->print("Reseteando...");
    dma_display->flipDMABuffer();

    WiFiManager wm;
    wm.resetSettings();
    nvsSaveStr("mqtt_host", "");
    nvsSaveStr("mqtt_user", "");
    nvsSaveStr("mqtt_pass", "");
    nvsSaveInt("mqtt_port", 1883);
    Serial.println("Factory reset — reiniciando");

    while (digitalRead(PIN_BOOT) == LOW) delay(100);
    delay(500);
    ESP.restart();
  }
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);
  nvsInit();

  // Display
  initDisplay();
  dma_display->setPanelBrightness(PANEL_BRIGHTNESS);

  // ─── WiFiManager con campos MQTT ────────────────────
  WiFiManager wm;
  wm.setConfigPortalTimeout(180);
  wm.setTitle("PixelClock");
  wm.setConnectTimeout(15);

  // Campos personalizados para MQTT
  char mqttHost[64] = "";
  char mqttPort[8]  = "";
  char mqttUser[32] = "";
  char mqttPass[32] = "";

  // Precargar valores desde NVS (si existen)
  nvsLoadStr("mqtt_host", "").toCharArray(mqttHost, sizeof(mqttHost));
  snprintf(mqttPort, sizeof(mqttPort), "%d", nvsLoadInt("mqtt_port", 1883));
  nvsLoadStr("mqtt_user", "").toCharArray(mqttUser, sizeof(mqttUser));
  nvsLoadStr("mqtt_pass", "").toCharArray(mqttPass, sizeof(mqttPass));

  WiFiManagerParameter pMqttHost("mqtt_host", "Servidor MQTT (IP o host)", mqttHost, 63);
  WiFiManagerParameter pMqttPort("mqtt_port", "Puerto MQTT", mqttPort, 7);
  WiFiManagerParameter pMqttUser("mqtt_user", "Usuario MQTT", mqttUser, 31);
  WiFiManagerParameter pMqttPass("mqtt_pass", "Contrasena MQTT", mqttPass, 31, "type=password");

  wm.addParameter(&pMqttHost);
  wm.addParameter(&pMqttPort);
  wm.addParameter(&pMqttUser);
  wm.addParameter(&pMqttPass);

  // Mostrar QR mientras espera config
  showQRCode();

  if (!wm.autoConnect("PixelClock-AP")) {
    Serial.println("WiFi timeout — reiniciando");
    ESP.restart();
  }

  // Guardar config MQTT en NVS tras configuracion
  nvsSaveStr("mqtt_host", pMqttHost.getValue());
  nvsSaveInt("mqtt_port", atoi(pMqttPort.getValue()));
  nvsSaveStr("mqtt_user", pMqttUser.getValue());
  nvsSaveStr("mqtt_pass", pMqttPass.getValue());

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

  // Comprobar BOOT cada ~500ms para factory reset
  static unsigned long lastBootCheck = 0;
  if (millis() - lastBootCheck > 500) {
    lastBootCheck = millis();
    checkFactoryReset();
  }

  delay(10);
}
