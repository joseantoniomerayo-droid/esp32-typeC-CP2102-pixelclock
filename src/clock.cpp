#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <HTTPClient.h>
#include "clock.h"
#include "display.h"
#include "credentials.h"

// ─── Config ────────────────────────────────────────────
struct Weather {
  int code;
  float temp;
  bool valid;
  unsigned long updated;
};
static Weather weather = {0, 0, false, 0};

static unsigned long lastWeatherFetch = 0;
const unsigned long WEATHER_INTERVAL = 30UL * 60 * 1000;

// ─── WiFi ──────────────────────────────────────────────
void initWiFi() {
  Serial.printf("WiFi: %s ... ", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  int a = 0;
  while (WiFi.status() != WL_CONNECTED && a < 40) { delay(500); Serial.print("."); a++; }
  if (WiFi.status() == WL_CONNECTED)
    Serial.printf("\nIP: %s\n", WiFi.localIP().toString().c_str());
  else
    Serial.println("\nFAIL");
}

// ─── NTP ───────────────────────────────────────────────
bool syncNTP() {
  if (WiFi.status() != WL_CONNECTED) return false;
  configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org", "time.nist.gov");
  struct tm t;
  if (getLocalTime(&t, 4000)) {
    Serial.printf("NTP: %02d:%02d\n", t.tm_hour, t.tm_min);
    return true;
  }
  Serial.println("NTP FAIL");
  return false;
}

// ─── Weather fetch (Open-Meteo) ────────────────────────
// Sin ArduinoJson — parse manual para evitar crashes
void fetchWeather() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.setTimeout(10000);

  char url[256];
  snprintf(url, sizeof(url),
    "http://api.open-meteo.com/v1/forecast"
    "?latitude=40.4168&longitude=-3.7038"
    "&current_weather=true&timezone=auto");

  if (!http.begin(url)) {
    Serial.println("Weather: begin fail");
    return;
  }

  int code = http.GET();

  if (code == 200) {
    String payload = http.getString();

    // Buscar dentro de "current_weather": { ... }
    int cw = payload.indexOf("\"current_weather\":");
    if (cw < 0) { weather.valid = false; Serial.println("Weather: no current_weather"); http.end(); return; }

    // Buscar "temperature": dentro de current_weather
    int tempPos = payload.indexOf("\"temperature\":", cw);
    if (tempPos >= 0) {
      int s = tempPos + 14;
      int e = payload.indexOf(',', s);
      if (e < 0) e = payload.indexOf('}', s);
      if (e > s) weather.temp = payload.substring(s, e).toFloat();
    }

    // Buscar "weathercode": dentro de current_weather
    int wcPos = payload.indexOf("\"weathercode\":", cw);
    if (wcPos >= 0) {
      int s = wcPos + 13;
      int e = payload.indexOf(',', s);
      if (e < 0) e = payload.indexOf('}', s);
      if (e > s) weather.code = payload.substring(s, e).toInt();
    }

    weather.valid = (tempPos >= 0);
    weather.updated = millis();
    Serial.printf("Weather: code=%d %.1f°C\n", weather.code, weather.temp);
  } else {
    Serial.printf("Weather HTTP fail: %d\n", code);
  }
  http.end();
}

// ════════════════════════════════════════════════════════
//  RENDER — ESTILO RELOJPIXEL ORIGINAL
// ════════════════════════════════════════════════════════

// ─── HSV → RGB565 ──────────────────────────────────────
static uint16_t hsvTo565Full(int h, float s, float v) {
  while (h < 0) h += 360;
  h = h % 360;
  float c = v * s;
  float x = c * (1 - fabsf(fmodf(h / 60.0f, 2) - 1));
  float m = v - c;
  float r, g, b;
  if (h < 60)       { r = c; g = x; b = 0; }
  else if (h < 120) { r = x; g = c; b = 0; }
  else if (h < 180) { r = 0; g = c; b = x; }
  else if (h < 240) { r = 0; g = x; b = c; }
  else if (h < 300) { r = x; g = 0; b = c; }
  else              { r = c; g = 0; b = x; }
  uint8_t R = (uint8_t)((r + m) * 255);
  uint8_t G = (uint8_t)((g + m) * 255);
  uint8_t B = (uint8_t)((b + m) * 255);
  return ((R & 0xF8) << 8) | ((G & 0xFC) << 3) | (B >> 3);
}

static uint16_t hsvTo565(int h) {
  return hsvTo565Full(h, 1.0f, 1.0f);
}

// Gradiente del original: amarillo (h=80) → rojo (h=0)
static uint16_t gradientColor(int x, int width) {
  float t = (float)x / (float)(width - 1);
  int h = (int)(80 - 80 * t);
  if (h < 0) h = 0;
  return hsvTo565(h);
}

// ─── Iconos clima 8×8 ─────────────────────────────────
static const uint8_t sunIcon[8] = {
  0b10010010, 0b01000100, 0b00011000, 0b10111101,
  0b00011000, 0b01000100, 0b10010010, 0b00000000,
};
static const uint8_t cloudyIcon[8] = {
  0b00000000, 0b00111100, 0b01111110, 0b11111111,
  0b11111111, 0b01111110, 0b00000000, 0b00000000,
};
static const uint8_t rainIcon[8] = {
  0b00111100, 0b01111110, 0b11111111, 0b01111110,
  0b00000000, 0b01001001, 0b01001001, 0b00000000,
};
static const uint8_t snowIcon[8] = {
  0b00010000, 0b01010100, 0b00101000, 0b11010110,
  0b00101000, 0b01010100, 0b00010000, 0b00000000,
};
static const uint8_t fogIcon[8] = {
  0b00000000, 0b11111110, 0b00000000, 0b01111110,
  0b00000000, 0b11111110, 0b00000000, 0b01111100,
};
static const uint8_t stormIcon[8] = {
  0b00111100, 0b01111110, 0b11111111, 0b11111111,
  0b01000010, 0b00100100, 0b00011000, 0b00100100,
};

static const uint8_t* pickWeatherIcon(int wmoCode) {
  if (wmoCode == 0 || wmoCode == 1) return sunIcon;
  if (wmoCode <= 3) return cloudyIcon;
  if (wmoCode <= 48) return fogIcon;
  if (wmoCode <= 67) return rainIcon;
  if (wmoCode <= 77) return snowIcon;
  if (wmoCode <= 82) return rainIcon;
  if (wmoCode >= 95) return stormIcon;
  return cloudyIcon;
}

static uint16_t weatherIconColor(int wmoCode) {
  if (wmoCode == 0 || wmoCode == 1) return dma_display->color565(255, 200, 0);   // sol: amarillo
  if (wmoCode <= 3) return dma_display->color565(200, 200, 200);                   // nube: gris
  if (wmoCode <= 48) return dma_display->color565(180, 180, 180);                  // niebla: gris claro
  if (wmoCode <= 67) return dma_display->color565(100, 150, 255);                  // lluvia: azul
  if (wmoCode <= 77) return dma_display->color565(220, 240, 255);                  // nieve: blanco
  if (wmoCode >= 95) return dma_display->color565(180, 100, 255);                  // tormenta: violeta
  return dma_display->color565(200, 200, 200);
}

// ─── Clima (esquina superior derecha) ──────────────────
static void drawWeatherWidget() {
  if (!weather.valid) return;

  const uint8_t* icon = pickWeatherIcon(weather.code);
  uint16_t col = weatherIconColor(weather.code);

  int tempWidth = 24;
  int iconX = PANEL_RES_X * PANEL_CHAIN - 8 - 2 - tempWidth;  // 94
  int iconY = 1;

  // Dibujar icono
  for (int row = 0; row < 8; row++) {
    for (int colB = 0; colB < 8; colB++) {
      if (icon[row] & (1 << (7 - colB))) {
        dma_display->drawPixel(iconX + colB, iconY + row, col);
      }
    }
  }

  // Temperatura
  dma_display->setTextSize(1);
  dma_display->setTextColor(dma_display->color565(255, 255, 255));
  dma_display->setCursor(iconX + 10, 2);
  dma_display->printf("%.0f", weather.temp);
  dma_display->print((char)247);  // °
  dma_display->print("C");
}

// ─── Pantalla "Conectando..." ──────────────────────────
void showConnecting(const char* msg) {
  dma_display->fillScreen(0);
  dma_display->setTextSize(1);
  dma_display->setTextColor(dma_display->color565(100, 150, 255));
  dma_display->setCursor(10, 12);
  dma_display->print(msg);
  dma_display->flipDMABuffer();
}

// ─── Renderiza el reloj (estilo original) ──────────────
static unsigned long lastDraw = 0;
static int dayDateScroll = 0;
static unsigned long lastScrollUpdate = 0;

void drawClock() {
  if (millis() - lastDraw < 60) return;  // ~16 fps como el original
  lastDraw = millis();

  // Weather refresh cada 30 min
  if (WiFi.status() == WL_CONNECTED &&
      (millis() - lastWeatherFetch > WEATHER_INTERVAL || !weather.valid)) {
    fetchWeather();
    lastWeatherFetch = millis();
  }

  struct tm t;
  bool timeOk = getLocalTime(&t, 50);
  int w = PANEL_RES_X * PANEL_CHAIN;  // 128

  dma_display->fillScreen(0);

  if (!timeOk) {
    dma_display->setCursor(2, 10);
    dma_display->setTextSize(1);
    dma_display->setTextColor(dma_display->color565(255, 80, 80));
    dma_display->print("Sin hora");
    dma_display->flipDMABuffer();
    return;
  }

  // ─── HORA: HH:MM:SS — textSize=3, gradiente amarillo→rojo ──
  char timeStr[9];
  strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &t);

  const int textSize = 3;
  const int charW = 5 * textSize + 1;  // 16px
  const int len = 8;
  const int totalW = len * charW;      // 128 — ocupa todo el ancho
  int originX = 0;                     // (128-128)/2 = 0, pegado a la izquierda
  int originY = 10;                    // igual que el original

  dma_display->setTextSize(textSize);
  for (int idx = 0; idx < len; idx++) {
    int charX = originX + idx * charW;
    uint16_t col = gradientColor(charX + charW / 2, w);
    dma_display->setTextColor(col);
    dma_display->setCursor(charX, originY);
    dma_display->print(timeStr[idx]);
  }

  // ─── CLIMA: esquina superior derecha ─────────────────────
  drawWeatherWidget();

  // ─── MARQUEE INFERIOR: día + fecha (scroll suave) ────────
  const char* days[] = {"domingo","lunes","martes","miercoles","jueves","viernes","sabado"};
  char marquee[32];
  snprintf(marquee, sizeof(marquee), "%s %02d-%02d-%04d",
           days[t.tm_wday], t.tm_mday, t.tm_mon + 1, t.tm_year + 1900);

  int marqueeW = strlen(marquee) * 6;
  int screenW = w;

  dma_display->setTextSize(1);
  dma_display->setTextColor(dma_display->color565(180, 180, 180));

  if (marqueeW > screenW) {
    // Scroll si el texto es más ancho que la pantalla
    if (millis() - lastScrollUpdate > 30) {
      lastScrollUpdate = millis();
      dayDateScroll--;
      if (dayDateScroll < -marqueeW) dayDateScroll = screenW;
    }
    dma_display->setCursor(dayDateScroll, PANEL_RES_Y - 8);
    dma_display->print(marquee);
  } else {
    // Centrado si cabe
    dma_display->setCursor((screenW - marqueeW) / 2, PANEL_RES_Y - 8);
    dma_display->print(marquee);
  }

  dma_display->flipDMABuffer();
}

// ─── Debug: forzar datos de clima ─────────────────────
void debugSetWeather(int code, float temp) {
  weather.code = code;
  weather.temp = temp;
  weather.valid = true;
  Serial.printf("DEBUG weather: code=%d %.1f°C\n", code, temp);
}
