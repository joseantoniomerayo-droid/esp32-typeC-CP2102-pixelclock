#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "mqtt_handler.h"
#include "clock.h"
#include "nvs_config.h"

static WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

static char mqttHost[64] = "";
static int  mqttPort = 1883;
static char mqttClientId[32] = "pixelclock";
static char mqttUser[32] = "";
static char mqttPass[32] = "";

static unsigned long lastReconnect = 0;
static unsigned long lastStatusPub = 0;

// ─── Callback MQTT ──────────────────────────────────────
static void mqttCallback(char* topic, byte* payload, unsigned int len) {
  char buf[256];
  unsigned int clen = len < sizeof(buf)-1 ? len : sizeof(buf)-1;
  memcpy(buf, payload, clen);
  buf[clen] = '\0';

  Serial.printf("[MQTT] %s\n", topic);

  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, buf);
  if (err) return;

  if (doc.containsKey("brillo_dia"))    setBrilloDia(doc["brillo_dia"]);
  if (doc.containsKey("brillo_noche"))  setBrilloNoche(doc["brillo_noche"]);
  if (doc.containsKey("inicio_noche"))  setInicioNoche(doc["inicio_noche"]);
  if (doc.containsKey("fin_noche"))     setFinNoche(doc["fin_noche"]);
  if (doc.containsKey("gradiente"))     setGradiente(doc["gradiente"]);
  if (doc.containsKey("gradiente_noche")) setGradienteNoche(doc["gradiente_noche"]);
  if (doc.containsKey("formato_fecha"))   setFormatoFecha(doc["formato_fecha"]);
  if (doc.containsKey("mostrar_dia"))    setMostrarDia(doc["mostrar_dia"]);
  if (doc.containsKey("marquee_activo")) setMarqueeActivo(doc["marquee_activo"]);
  if (doc.containsKey("grad_ini")) setColorInicio(false, strtoul(((const char*)doc["grad_ini"])+1, NULL, 16));
  if (doc.containsKey("grad_fin")) setColorFin(false, strtoul(((const char*)doc["grad_fin"])+1, NULL, 16));
  if (doc.containsKey("grad_noc_ini")) setColorInicio(true, strtoul(((const char*)doc["grad_noc_ini"])+1, NULL, 16));
  if (doc.containsKey("grad_noc_fin")) setColorFin(true, strtoul(((const char*)doc["grad_noc_fin"])+1, NULL, 16));
  if (doc.containsKey("clima_refresh")) setClimaRefresh(doc["clima_refresh"]);
  if (doc.containsKey("clima_lat"))     setLatitud(doc["clima_lat"]);
  if (doc.containsKey("clima_lon"))     setLongitud(doc["clima_lon"]);

  mqttPublishStatus();
}

// ─── Publicar estado ────────────────────────────────────
void mqttPublishStatus() {
  if (!mqttClient.connected()) return;

  char buf[384];
  snprintf(buf, sizeof(buf),
    "{\"brillo_dia\":%d,\"brillo_noche\":%d,"
    "\"inicio_noche\":%d,\"fin_noche\":%d,"
    "\"gradiente\":%d,\"gradiente_noche\":%d,\"formato_fecha\":%d,\"mostrar_dia\":%d,\"marquee_activo\":%d,"
    "\"grad_ini\":\"#%06X\",\"grad_fin\":\"#%06X\","
    "\"grad_noc_ini\":\"#%06X\",\"grad_noc_fin\":\"#%06X\","
    "\"clima_refresh\":%d,"
    "\"clima_lat\":%.4f,\"clima_lon\":%.4f}",
    getBrilloDia(), getBrilloNoche(),
    getInicioNoche(), getFinNoche(),
    getGradiente(), getGradienteNoche(), getFormatoFecha(), getMostrarDia()?1:0, getMarqueeActivo()?1:0,
    getColorInicio(false), getColorFin(false),
    getColorInicio(true), getColorFin(true), getClimaRefresh(),
    getLatitud(), getLongitud());

  mqttClient.publish("reloj/status", buf, true);
}

// ─── Init MQTT ──────────────────────────────────────────
void initMQTT() {
  String host = nvsLoadStr("mqtt_host", "");
  if (host.length() == 0) {
    Serial.println("MQTT: no configurado (sin host)");
    return;
  }

  snprintf(mqttHost, sizeof(mqttHost), "%s", host.c_str());
  mqttPort = nvsLoadInt("mqtt_port", 1883);

  String user = nvsLoadStr("mqtt_user", "");
  String pass = nvsLoadStr("mqtt_pass", "");
  snprintf(mqttUser, sizeof(mqttUser), "%s", user.c_str());
  snprintf(mqttPass, sizeof(mqttPass), "%s", pass.c_str());

  mqttClient.setServer(mqttHost, mqttPort);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(512);

  Serial.printf("MQTT: %s:%d\n", mqttHost, mqttPort);
}

// ─── Loop MQTT ──────────────────────────────────────────
void mqttLoop() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (strlen(mqttHost) == 0) return;

  if (!mqttClient.connected()) {
    unsigned long now = millis();
    if (now - lastReconnect > 30000) {
      lastReconnect = now;
      Serial.printf("MQTT: conectando... ");

      bool ok;
      if (strlen(mqttUser) > 0)
        ok = mqttClient.connect(mqttClientId, mqttUser, mqttPass, "reloj/status", 1, true, "{\"mqtt\":\"err\"}");
      else
        ok = mqttClient.connect(mqttClientId, "reloj/status", 1, true, "{\"mqtt\":\"err\"}");

      if (ok) {
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

  if (millis() - lastStatusPub > 300000) {
    lastStatusPub = millis();
    mqttPublishStatus();
  }
}

bool isMQTTConnected() { return mqttClient.connected(); }
