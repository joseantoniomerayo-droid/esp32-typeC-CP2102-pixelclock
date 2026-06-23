#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <HTTPClient.h>
#include "clock.h"
#include "display.h"
#include "nvs_config.h"

// ─── Config ────────────────────────────────────────────
struct Weather {
  int code;
  float temp;
  bool valid;
  unsigned long updated;
};
static Weather weather = {0, 0, false, 0};

static unsigned long lastWeatherFetch = 0;
static unsigned long weatherInterval = 0;

// ─── NTP ───────────────────────────────────────────────
bool syncNTP() {
  if (WiFi.status() != WL_CONNECTED) return false;
  configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org", "time.nist.gov");
  struct tm t;
  if (getLocalTime(&t, 4000)) return true;
  return false;
}

// ─── Weather fetch (Open-Meteo) ────────────────────────
void fetchWeather() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.setTimeout(10000);

  char url[256];
  snprintf(url, sizeof(url),
    "http://api.open-meteo.com/v1/forecast"
    "?latitude=%.4f&longitude=%.4f"
    "&current_weather=true&timezone=auto",
    getLatitud(), getLongitud());

  if (!http.begin(url)) return;
  int code = http.GET();

  if (code == 200) {
    String payload = http.getString();

    int cw = payload.indexOf("\"current_weather\":");
    if (cw < 0) { weather.valid = false; http.end(); return; }

    int tempPos = payload.indexOf("\"temperature\":", cw);
    if (tempPos >= 0) {
      int s = tempPos + 14;
      int e = payload.indexOf(',', s);
      if (e < 0) e = payload.indexOf('}', s);
      if (e > s) weather.temp = payload.substring(s, e).toFloat();
    }

    int wcPos = payload.indexOf("\"weathercode\":", cw);
    if (wcPos >= 0) {
      int s = wcPos + 13;
      int e = payload.indexOf(',', s);
      if (e < 0) e = payload.indexOf('}', s);
      if (e > s) weather.code = payload.substring(s, e).toInt();
    }

    weather.valid = (tempPos >= 0);
    weather.updated = millis();

    // Ajustar zona horaria desde la respuesta de Open-Meteo
    int tz = payload.indexOf("\"timezone_abbreviation\":\"");
    if (tz >= 0) {
      int s = tz + 25;
      int e = payload.indexOf('"', s);
      if (e > s) {
        String tzStr = payload.substring(s, e);
        if (tzStr == "GMT+2" || tzStr == "CEST") {
          setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
        } else if (tzStr == "GMT" || tzStr == "GMT+0" || tzStr == "GMT+1" || tzStr == "CET") {
          setenv("TZ", "CET-1", 1);
        } else if (tzStr == "GMT-5" || tzStr == "EST") {
          setenv("TZ", "EST+5", 1);
        } else if (tzStr == "GMT-8" || tzStr == "PST") {
          setenv("TZ", "PST+8", 1);
        }
        tzset();
      }
    }
  }
  http.end();
}

// ════════════════════════════════════════════════════════
//  RENDER
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

// Gradiente por interpolacion entre dos colores configurados en NVS
// formato: RRGGBB hex. Si inicio==fin, color solido.
static uint16_t gradientColor(int x, int width, bool esNoche) {
  uint32_t cStart = getColorInicio(esNoche);
  uint32_t cEnd   = getColorFin(esNoche);
  float t = (float)x / (float)(width - 1);

  uint8_t r = ((cStart >> 16) & 0xFF) * (1.0f - t) + ((cEnd >> 16) & 0xFF) * t;
  uint8_t g = ((cStart >> 8)  & 0xFF) * (1.0f - t) + ((cEnd >> 8)  & 0xFF) * t;
  uint8_t b = (cStart        & 0xFF) * (1.0f - t) + (cEnd        & 0xFF) * t;

  return dma_display->color565(r, g, b);
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

// ─── Fuente 4×6 (columnas x filas) ──────────────────────
// Cada caracter = 4 columnas, 6 filas (bit 0 = arriba)
// Solo mayusculas, digitos, espacio, dos puntos, guion, barra
#define FONT_W 4
#define FONT_H 6

// Indices: 0=espacio, 1-10=0-9, 11-36=A-Z, 37=:, 38=-, 39=/
// Tabla de conversion: caracter -> indice
static int charIdx(char c) {
  if (c == ' ') return 0;
  if (c >= '0' && c <= '9') return 1 + (c - '0');
  if (c >= 'A' && c <= 'Z') return 11 + (c - 'A');
  if (c >= 'a' && c <= 'z') return 11 + (c - 'a');
  if (c == ':') return 37;
  if (c == '-') return 38;
  if (c == '/') return 39;
  return 0; // fallback a espacio
}

// 40 caracteres x 4 columnas = 160 bytes
static const uint8_t tinyFont[40][4] = {
  {0x00,0x00,0x00,0x00}, // 0 espacio
  {0x00,0x7C,0x00,0x00}, // 1 0
  {0x00,0x24,0x7C,0x00}, // 2 1
  {0x44,0x64,0x54,0x4C}, // 3 2
  {0x44,0x54,0x54,0x28}, // 4 3
  {0x70,0x10,0x10,0x7C}, // 5 4
  {0x74,0x54,0x54,0x28}, // 6 5
  {0x7C,0x44,0x44,0x7C}, // 7 6
  {0x7C,0x44,0x44,0x38}, // 8 7
  {0x04,0x04,0x7C,0x04}, // 9 8
  {0x38,0x44,0x44,0x7C}, // 10 9
  {0x7C,0x20,0x20,0x7C}, // 11 A
  {0x7C,0x54,0x54,0x28}, // 12 B
  {0x38,0x44,0x44,0x28}, // 13 C
  {0x7C,0x44,0x44,0x38}, // 14 D
  {0x7C,0x54,0x54,0x44}, // 15 E
  {0x7C,0x14,0x14,0x04}, // 16 F
  {0x38,0x44,0x54,0x78}, // 17 G
  {0x7C,0x10,0x10,0x7C}, // 18 H
  {0x44,0x7C,0x44,0x00}, // 19 I
  {0x28,0x44,0x44,0x3C}, // 20 J
  {0x7C,0x10,0x28,0x44}, // 21 K
  {0x7C,0x40,0x40,0x40}, // 22 L
  {0x7C,0x08,0x10,0x7C}, // 23 M
  {0x7C,0x08,0x20,0x7C}, // 24 N
  {0x38,0x44,0x44,0x38}, // 25 O
  {0x7C,0x24,0x24,0x18}, // 26 P
  {0x38,0x44,0x24,0x58}, // 27 Q
  {0x7C,0x24,0x34,0x48}, // 28 R
  {0x48,0x54,0x54,0x24}, // 29 S
  {0x04,0x7C,0x04,0x00}, // 30 T
  {0x3C,0x40,0x40,0x3C}, // 31 U
  {0x1C,0x60,0x60,0x1C}, // 32 V
  {0x7C,0x30,0x30,0x7C}, // 33 W
  {0x44,0x28,0x10,0x28}, // 34 X
  {0x44,0x28,0x10,0x7C}, // 35 Y
  {0x44,0x64,0x54,0x4C}, // 36 Z
  {0x00,0x48,0x00,0x00}, // 37 :
  {0x00,0x10,0x10,0x00}, // 38 -
  {0x40,0x20,0x10,0x08}, // 39 /
};

// Renderiza texto en fuente 4x6 en (x,y)
static void drawTinyText(int x, int y, const char* text, uint16_t color) {
  while (*text) {
    int idx = charIdx(*text);
    for (int col = 0; col < FONT_W; col++) {
      uint8_t bits = tinyFont[idx][col];
      for (int row = 0; row < FONT_H; row++) {
        if (bits & (1 << row))
          dma_display->drawPixel(x + col, y + row, color);
      }
    }
    x += FONT_W + 1; // 1px interletrado
    text++;
  }
}

// ─── Icono calendario 8×8 ──────────────────────────────
static const uint8_t calendarIcon[8] = {
  0b01111110,
  0b01000010,
  0b11111111,
  0b10000001,
  0b10000001,
  0b10000001,
  0b10000001,
  0b11111111,
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
  if (wmoCode == 0 || wmoCode == 1) return dma_display->color565(255, 200, 0);
  if (wmoCode <= 3) return dma_display->color565(200, 200, 200);
  if (wmoCode <= 48) return dma_display->color565(180, 180, 180);
  if (wmoCode <= 67) return dma_display->color565(100, 150, 255);
  if (wmoCode <= 77) return dma_display->color565(220, 240, 255);
  if (wmoCode >= 95) return dma_display->color565(180, 100, 255);
  return dma_display->color565(200, 200, 200);
}

// ─── Clima (esquina superior derecha) ──────────────────
static void drawWeatherWidget() {
  if (!weather.valid) return;

  const uint8_t* icon = pickWeatherIcon(weather.code);
  uint16_t col = weatherIconColor(weather.code);

  int iconX = PANEL_RES_X * PANEL_CHAIN - 8 - 2 - 24;
  int iconY = 1;

  for (int row = 0; row < 8; row++)
    for (int colB = 0; colB < 8; colB++)
      if (icon[row] & (1 << (7 - colB)))
        dma_display->drawPixel(iconX + colB, iconY + row, col);

  dma_display->setTextSize(1);
  dma_display->setTextColor(dma_display->color565(255, 255, 255));
  dma_display->setCursor(iconX + 10, 2);
  dma_display->printf("%.0f", weather.temp);
  dma_display->print((char)247);
  dma_display->print("C");
}

// ─── Eventos calendario (recibidos por MQTT) ─────────────
#define MAX_EVENTS 8

static struct { char time[6]; char title[48]; } events[MAX_EVENTS];
static int eventCount = 0;
static int currentEventIdx = 0;
static unsigned long lastEventRotate = 0;

// Comprueba si la hora del evento ya ha pasado
static bool isTimePassed(const char* timeStr) {
  if (strlen(timeStr) == 0) return false; // evento de todo el dia
  if (strlen(timeStr) < 5) return false;

  struct tm t;
  if (!getLocalTime(&t, 50)) return false;

  int eh = (timeStr[0]-'0')*10 + (timeStr[1]-'0');
  int em = (timeStr[3]-'0')*10 + (timeStr[4]-'0');

  return (t.tm_hour > eh) || (t.tm_hour == eh && t.tm_min > em);
}

void setCalendarEvents(JsonArray arr) {
  eventCount = 0;
  currentEventIdx = 0;
  for (JsonVariant v : arr) {
    if (eventCount >= MAX_EVENTS) break;
    JsonArray e = v;
    const char* t = e[0] | "";
    // Saltar eventos cuya hora ya paso
    if (isTimePassed(t)) continue;
    strncpy(events[eventCount].time, t, 5);
    events[eventCount].time[5] = '\0';
    strncpy(events[eventCount].title, e[1] | "", sizeof(events[0].title)-1);
    Serial.printf("[EVENTS] #%d: %s %s\n", eventCount, events[eventCount].time, events[eventCount].title);
    eventCount++;
  }
  Serial.printf("[EVENTS] loaded %d events\n", eventCount);
}

// ─── Calendario (esquina superior izquierda) ────────────
static void drawCalendarWidget() {
  if (!getCalendarActivo() || eventCount == 0) return;

  // Rotar cada 5 segundos, saltando eventos pasados
  if (millis() - lastEventRotate > 5000) {
    lastEventRotate = millis();
    int tries = 0;
    do {
      currentEventIdx = (currentEventIdx + 1) % eventCount;
      tries++;
    } while (isTimePassed(events[currentEventIdx].time) && tries < eventCount);
    // Si todos han pasado, no mostrar nada
    if (isTimePassed(events[currentEventIdx].time)) return;
  }

  // Si el evento actual ya paso (solo ocurre si estaba visible al pasar la hora)
  if (isTimePassed(events[currentEventIdx].time)) return;

  const char* time = events[currentEventIdx].time;
  const char* title = events[currentEventIdx].title;

  // Icono calendario (cyan)
  uint16_t col = dma_display->color565(100, 200, 255);
  for (int row = 0; row < 8; row++)
    for (int c = 0; c < 8; c++)
      if (calendarIcon[row] & (1 << (7 - c)))
        dma_display->drawPixel(1 + c, 1 + row, col);

  // Texto (fuente 6x8 estandar, truncado si invade clima)
  // Weather empieza en x=94, dejamos margen hasta x=90
  char label[80];
  if (strlen(time) > 0)
    snprintf(label, sizeof(label), "%s %s", time, title);
  else
    snprintf(label, sizeof(label), "%s", title);

  // Truncar si ocupa mas de 80px (13 chars ~ 78px)
  int maxW = 80;
  int textW = strlen(label) * 6;
  if (textW > maxW) {
    int maxChars = (maxW - 12) / 6; // reservar 12px para ".."
    if (maxChars < 1) maxChars = 1;
    label[maxChars] = '\0';
    strcat(label, "..");
  }

  dma_display->setTextSize(1);
  dma_display->setTextColor(col);
  dma_display->setCursor(10, 2);
  dma_display->print(label);
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

// ─── Renderiza el reloj ────────────────────────────────
static unsigned long lastDraw = 0;
static int dayDateScroll = 0;
static unsigned long lastScrollUpdate = 0;

void drawClock() {
  if (millis() - lastDraw < 60) return;
  lastDraw = millis();

  weatherInterval = (unsigned long)getClimaRefresh() * 60 * 1000;
  if (WiFi.status() == WL_CONNECTED &&
      (millis() - lastWeatherFetch > weatherInterval || !weather.valid)) {
    fetchWeather();
    lastWeatherFetch = millis();
  }

  struct tm t;
  bool timeOk = getLocalTime(&t, 50);
  int w = PANEL_RES_X * PANEL_CHAIN;

  dma_display->fillScreen(0);

  if (!timeOk) {
    dma_display->setCursor(2, 10);
    dma_display->setTextSize(1);
    dma_display->setTextColor(dma_display->color565(255, 80, 80));
    dma_display->print("Sin hora");
    dma_display->flipDMABuffer();
    return;
  }

  // ─── Brillo automático ──────────────────────────────
  static int lastBrightness = -1;
  bool esNoche = getUsarNocturno() && (t.tm_hour >= nvsLoadInt("inicio_noche", 23) || t.tm_hour < nvsLoadInt("fin_noche", 7));
  int targetBrightness = esNoche ? nvsLoadInt("brillo_noche", 1) : nvsLoadInt("brillo_dia", 40);
  if (targetBrightness != lastBrightness) {
    dma_display->setPanelBrightness(targetBrightness);
    lastBrightness = targetBrightness;
  }

  // ─── HORA: HH:MM:SS — textSize=3 ────────────────────
  char timeStr[9];
  strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &t);

  dma_display->setTextSize(3);
  for (int idx = 0; idx < 8; idx++) {
    int charX = idx * 16;
    dma_display->setTextColor(gradientColor(charX + 8, w, esNoche));
    dma_display->setCursor(charX, 10);
    dma_display->print(timeStr[idx]);
  }

  // ─── CLIMA ──────────────────────────────────────────
  drawWeatherWidget();

  // ─── CALENDARIO ─────────────────────────────────────
  drawCalendarWidget();

  // ─── MARQUEE INFERIOR: día + fecha ──────────────────
  if (!getMarqueeActivo()) { dma_display->flipDMABuffer(); return; }

  const char* days[] = {"domingo","lunes","martes","miercoles","jueves","viernes","sabado"};
  char marquee[32];

  int fmt = getFormatoFecha();
  bool mostrarDia = getMostrarDia();

  if (mostrarDia) {
    switch (fmt) {
      case 1: snprintf(marquee, sizeof(marquee), "%s %02d/%02d/%04d", days[t.tm_wday], t.tm_mday, t.tm_mon+1, t.tm_year+1900); break;
      case 2: snprintf(marquee, sizeof(marquee), "%s %02d-%02d-%04d", days[t.tm_wday], t.tm_mon+1, t.tm_mday, t.tm_year+1900); break;
      case 3: snprintf(marquee, sizeof(marquee), "%s %04d-%02d-%02d", days[t.tm_wday], t.tm_year+1900, t.tm_mon+1, t.tm_mday); break;
      default: snprintf(marquee, sizeof(marquee), "%s %02d-%02d-%04d", days[t.tm_wday], t.tm_mday, t.tm_mon+1, t.tm_year+1900); break;
    }
  } else {
    switch (fmt) {
      case 1: snprintf(marquee, sizeof(marquee), "%02d/%02d/%04d", t.tm_mday, t.tm_mon+1, t.tm_year+1900); break;
      case 2: snprintf(marquee, sizeof(marquee), "%02d-%02d-%04d", t.tm_mon+1, t.tm_mday, t.tm_year+1900); break;
      case 3: snprintf(marquee, sizeof(marquee), "%04d-%02d-%02d", t.tm_year+1900, t.tm_mon+1, t.tm_mday); break;
      default: snprintf(marquee, sizeof(marquee), "%02d-%02d-%04d", t.tm_mday, t.tm_mon+1, t.tm_year+1900); break;
    }
  }

  int marqueeW = strlen(marquee) * 6;
  int centerX = (w - marqueeW) / 2;

  dma_display->setTextSize(1);
  dma_display->setTextColor(dma_display->color565(180, 180, 180));

  static unsigned long animStart = 0;
  if (animStart == 0) animStart = millis();

  if (marqueeW > w) {
    if (millis() - lastScrollUpdate > 30) {
      lastScrollUpdate = millis();
      dayDateScroll--;
      if (dayDateScroll < -marqueeW) dayDateScroll = w;
    }
    dma_display->setCursor(dayDateScroll, PANEL_RES_Y - 8);
    dma_display->print(marquee);
  } else {
    const unsigned long CYCLE = 15000;
    const unsigned long SLIDE_IN = 600;
    const unsigned long HOLD = 11000;
    const unsigned long SLIDE_OUT = 600;

    unsigned long phase = (millis() - animStart) % CYCLE;

    if (phase < SLIDE_IN) {
      float p = (float)phase / SLIDE_IN;
      float ease = 1.0f - (1.0f - p) * (1.0f - p);
      int x = w + (int)((centerX - w) * ease);
      dma_display->setCursor(x, PANEL_RES_Y - 8);
      dma_display->print(marquee);
    } else if (phase < SLIDE_IN + HOLD) {
      dma_display->setCursor(centerX, PANEL_RES_Y - 8);
      dma_display->print(marquee);
    } else if (phase < SLIDE_IN + HOLD + SLIDE_OUT) {
      float p = (float)(phase - SLIDE_IN - HOLD) / SLIDE_OUT;
      float ease = p * p;
      int x = centerX + (int)((-marqueeW - centerX) * ease);
      dma_display->setCursor(x, PANEL_RES_Y - 8);
      dma_display->print(marquee);
    }
  }

  dma_display->flipDMABuffer();
}
