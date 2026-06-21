#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "mqtt_handler.h"
#include "clock.h"
#include "nvs_config.h"

// ─── Broker ────────────────────────────────────────────
// Estos valores se pueden cambiar por MQTT o por NVS
static char mqttHost[64] = "192.168.2.100";
static int  mqttPort = 1883;
static char mqttClientId[32] = "pixelclock";
static char mqttUser[32] = "";
static char mqttPass[32] = "";

static WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

static unsigned long lastReconnect = 0;
static unsigned long lastStatusPub = 0;

// ─── Callback MQTT ──────────────────────────────────────
static void mqttCallback(char* topic, byte* payload, unsigned int len) {
  // Copiar payload a string
  char buf[256];
  unsigned int clen = len < sizeof(buf)-1 ? len : sizeof(buf)-1;
  memcpy(buf, payload, clen);
  buf[clen] = '\0';

  Serial.printf("[MQTT] %s → %s\n", topic, buf);

  // Parsear JSON
  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, buf);
  if (err) { Serial.printf("MQTT JSON err: %s\n", err.c_str()); return; }

  if (doc.containsKey("brillo_dia"))    setBrilloDia(doc["brillo_dia"]);
  if (doc.containsKey("brillo_noche"))  setBrilloNoche(doc["brillo_noche"]);
  if (doc.containsKey("inicio_noche"))  setInicioNoche(doc["inicio_noche"]);
  if (doc.containsKey("fin_noche"))     setFinNoche(doc["fin_noche"]);
  if (doc.containsKey("gradiente"))     setGradiente(doc["gradiente"]);
  if (doc.containsKey("clima_refresh")) setClimaRefresh(doc["clima_refresh"]);
  if (doc.containsKey("clima_lat"))     setLatitud(doc["clima_lat"]);
  if (doc.containsKey("clima_lon"))     setLongitud(doc["clima_lon"]);
  if (doc.containsKey("mqtt_host")) {
    const char* h = doc["mqtt_host"];
    snprintf(mqttHost, sizeof(mqttHost), "%s", h);
    nvsSaveStr("mqtt_host", mqttHost);
  }

  // Responder con estado actual
  mqttPublishStatus();
}

// ─── Publicar estado ────────────────────────────────────
void mqttPublishStatus() {
  if (!mqttClient.connected()) return;

  char buf[384];
  snprintf(buf, sizeof(buf),
    "{\"brillo_dia\":%d,\"brillo_noche\":%d,"
    "\"inicio_noche\":%d,\"fin_noche\":%d,"
    "\"gradiente\":%d,\"clima_refresh\":%d,"
    "\"clima_lat\":%.4f,\"clima_lon\":%.4f,"
    "\"wifi\":\"%s\",\"mqtt\":\"ok\","
    "\"hora\":%lu}",
    getBrilloDia(), getBrilloNoche(),
    getInicioNoche(), getFinNoche(),
    getGradiente(), getClimaRefresh(),
    getLatitud(), getLongitud(),
    WiFi.status() == WL_CONNECTED ? "ok" : "no",
    millis() / 1000);

  mqttClient.publish("reloj/status", buf, true);
}

// ─── Init MQTT ──────────────────────────────────────────
void initMQTT() {
  // Cargar config MQTT desde NVS
  String h = nvsLoadStr("mqtt_host", "192.168.2.100");
  snprintf(mqttHost, sizeof(mqttHost), "%s", h.c_str());

  mqttClient.setServer(mqttHost, mqttPort);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(512);

  Serial.printf("[MQTT] Configurado: %s:%d\n", mqttHost, mqttPort);
}

// ─── Loop MQTT ──────────────────────────────────────────
void mqttLoop() {
  if (WiFi.status() != WL_CONNECTED) return;

  if (!mqttClient.connected()) {
    unsigned long now = millis();
    if (now - lastReconnect > 30000) {  // reintentar cada 30s
      lastReconnect = now;
      Serial.printf("[MQTT] Conectando como %s... ", mqttClientId);
      if (mqttClient.connect(mqttClientId, mqttUser, mqttPass, "reloj/status", 1, true, "{\"mqtt\":\"disconnected\"}")) {
        Serial.println("OK");
        mqttClient.subscribe("reloj/config");
        mqttPublishStatus();
      } else {
        Serial.printf("FAIL (rc=%d)\n", mqttClient.state());
      }
    }
    return;
  }

  mqttClient.loop();

  // Publicar status cada 5 minutos
  if (millis() - lastStatusPub > 300000) {
    lastStatusPub = millis();
    mqttPublishStatus();
  }
}

bool isMQTTConnected() { return mqttClient.connected(); }
