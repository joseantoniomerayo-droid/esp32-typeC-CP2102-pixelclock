#include <Arduino.h>
#include <NimBLEDevice.h>
#include <WiFi.h>
#include "ble_config.h"
#include "nvs_config.h"

// ─── UUIDs (mismos que WeatherEmoji) ───────────────────
static const char* SERVICE_UUID  = "fb349b5f-8000-0080-0010-0000e0ff0000";
static const char* CHAR_CMD_UUID = "fb349b5f-8000-0080-0010-0000e1ff0000";
static const char* CHAR_STS_UUID = "fb349b5f-8000-0080-0010-0000e2ff0000";

static NimBLECharacteristic* pStatusChar = nullptr;

// ─── Variables de WiFi (desde NVS) ─────────────────────
static char wifi_ssid[32] = "";
static char wifi_pass[64] = "";

// ─── Prototipos ────────────────────────────────────────
static void reconnectWiFi();

// ─── Envía respuesta por BLE ───────────────────────────
static void sendBLE(const char* msg) {
  if (pStatusChar) {
    pStatusChar->setValue(msg);
    pStatusChar->notify();
  }
}

// ─── Procesa comando recibido por BLE ──────────────────
static void processCmd(const char* cmd) {
  Serial.printf("[BLE] CMD: '%s'\n", cmd);

  if (strcmp(cmd, "status") == 0) {
    char buf[256];
    snprintf(buf, sizeof(buf),
      "{\"brillo_dia\":%d,\"brillo_noche\":%d,"
      "\"inicio_noche\":%d,\"fin_noche\":%d,"
      "\"gradiente\":%d,\"clima_refresh\":%d"
      "}",
      getBrilloDia(), getBrilloNoche(),
      getInicioNoche(), getFinNoche(),
      getGradiente(), getClimaRefresh());
    sendBLE(buf);
    return;
  }

  if (strcmp(cmd, "wificonnect") == 0) {
    sendBLE("OK: reconectando WiFi...");
    reconnectWiFi();
    return;
  }

  if (strncmp(cmd, "ssid:", 5) == 0) {
    snprintf(wifi_ssid, sizeof(wifi_ssid), "%.31s", cmd + 5);
    nvsSaveStr("wifi_ssid", wifi_ssid);
    sendBLE("OK: ssid guardado");
    return;
  }

  if (strncmp(cmd, "pass:", 5) == 0) {
    snprintf(wifi_pass, sizeof(wifi_pass), "%.63s", cmd + 5);
    nvsSaveStr("wifi_pass", wifi_pass);
    sendBLE("OK: pass guardado");
    return;
  }

  // ─── Parámetros numéricos ──────────────────────────
  int val;
  if (sscanf(cmd, "brillo_dia:%d", &val) == 1) {
    setBrilloDia(constrain(val, 0, 255));
    sendBLE("OK: brillo_dia");
    return;
  }
  if (sscanf(cmd, "brillo_noche:%d", &val) == 1) {
    setBrilloNoche(constrain(val, 0, 255));
    sendBLE("OK: brillo_noche");
    return;
  }
  if (sscanf(cmd, "inicio_noche:%d", &val) == 1) {
    setInicioNoche(constrain(val, 0, 23));
    sendBLE("OK: inicio_noche");
    return;
  }
  if (sscanf(cmd, "fin_noche:%d", &val) == 1) {
    setFinNoche(constrain(val, 0, 23));
    sendBLE("OK: fin_noche");
    return;
  }
  if (sscanf(cmd, "gradiente:%d", &val) == 1) {
    setGradiente(constrain(val, 0, 1));
    sendBLE("OK: gradiente");
    return;
  }
  if (sscanf(cmd, "clima_refresh:%d", &val) == 1) {
    setClimaRefresh(constrain(val, 5, 120));
    sendBLE("OK: clima_refresh");
    return;
  }

  // ─── Parámetros float ──────────────────────────────
  float f;
  if (sscanf(cmd, "clima_lat:%f", &f) == 1) {
    setLatitud(f);
    sendBLE("OK: clima_lat");
    return;
  }
  if (sscanf(cmd, "clima_lon:%f", &f) == 1) {
    setLongitud(f);
    sendBLE("OK: clima_lon");
    return;
  }

  sendBLE("ERROR: comando no valido");
}

// ─── Callback de escritura BLE ─────────────────────────
class CmdCallback : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pChar) {
    std::string val = pChar->getValue();
    if (val.length() > 0) {
      // Copiar y trim
      char buf[128];
      size_t len = val.length() < sizeof(buf)-1 ? val.length() : sizeof(buf)-1;
      memcpy(buf, val.data(), len);
      buf[len] = '\0';
      // Trim \r\n
      while (len > 0 && (buf[len-1]=='\n'||buf[len-1]=='\r'||buf[len-1]==' '))
        buf[--len] = '\0';
      processCmd(buf);
    }
  }
};

// ─── WiFi ──────────────────────────────────────────────
static void connectWiFi() {
  String ssid = nvsLoadStr("wifi_ssid", "");
  String pass = nvsLoadStr("wifi_pass", "");

  if (ssid.length() == 0) {
    Serial.println("WiFi: No hay credenciales en NVS");
    return;
  }

  snprintf(wifi_ssid, sizeof(wifi_ssid), "%s", ssid.c_str());
  snprintf(wifi_pass, sizeof(wifi_pass), "%s", pass.c_str());

  Serial.printf("WiFi: conectando a %s ...\n", wifi_ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_pass);

  int a = 0;
  while (WiFi.status() != WL_CONNECTED && a < 40) {
    delay(500);
    Serial.print(".");
    a++;
  }

  if (WiFi.status() == WL_CONNECTED)
    Serial.printf("\nIP: %s\n", WiFi.localIP().toString().c_str());
  else
    Serial.println("\nFAIL");
}

static void reconnectWiFi() {
  WiFi.disconnect();
  delay(500);
  connectWiFi();
}

// ─── Callback de conexión BLE ──────────────────────────
class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer) {
    Serial.println("[BLE] Conectado");
  }
  void onDisconnect(NimBLEServer* pServer) {
    Serial.println("[BLE] Desconectado — reanunciando");
    NimBLEDevice::startAdvertising();
  }
};

// ─── Init BLE ──────────────────────────────────────────
void initBLE() {
  NimBLEDevice::init("PixelClock");       // nombre visible
  NimBLEDevice::setPower(ESP_PWR_LVL_P9); // potencia máxima

  NimBLEServer* server = NimBLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());

  NimBLEService* svc = server->createService(SERVICE_UUID);

  // CMD (write)
  NimBLECharacteristic* cmdChar = svc->createCharacteristic(
    CHAR_CMD_UUID,
    NIMBLE_PROPERTY::WRITE
  );
  cmdChar->setCallbacks(new CmdCallback());

  // STATUS (notify)
  pStatusChar = svc->createCharacteristic(
    CHAR_STS_UUID,
    NIMBLE_PROPERTY::NOTIFY
  );

  svc->start();

  // Advertising
  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->addServiceUUID(SERVICE_UUID);
  adv->setAppearance(0x0000);
  adv->start();

  Serial.println("[BLE] PixelClock anunciado");

  // Conectar WiFi si hay credenciales
  connectWiFi();
}
