#include <Arduino.h>
#include "display.h"

// ─── Brillo global ──────────────────────────────────────
uint8_t panelBrightness = 40;  // 0-255

// ─── Definición de los colores de calibración ───────────
struct CalColor {
  const char *name;
  uint8_t r, g, b;
};

#define NUM_COLORS 10
const CalColor colors[NUM_COLORS] = {
  {"ROJO",     255,   0,   0},
  {"VERDE",      0, 255,   0},
  {"AZUL",       0,   0, 255},
  {"BLANCO",   255, 255, 255},
  {"AMARILLO", 255, 255,   0},
  {"CIAN",       0, 255, 255},
  {"MAGENTA",  255,   0, 255},
  {"NARANJA",  255, 128,   0},
  {"GRIS",     128, 128, 128},
  {"NEGRO",      0,   0,   0},
};

int currentColor = 0;
unsigned long lastChange = 0;
const int COLOR_DURATION_MS = 3000;  // 3s por color en modo automático
bool autoMode = true;

// ─── Muestra color a pantalla completa ──────────────────
void showColor(int idx) {
  const CalColor &c = colors[idx];

  // Llenar la pantalla completa con el color
  dma_display->fillScreen(dma_display->color565(c.r, c.g, c.b));

  // Mostrar el nombre en blanco (o negro si el fondo es muy claro)
  uint16_t textColor;
  int luminance = c.r * 299 + c.g * 587 + c.b * 114;
  if (luminance > 150000)
    textColor = dma_display->color565(0, 0, 0);      // negro sobre claro
  else
    textColor = dma_display->color565(255, 255, 255); // blanco sobre oscuro

  dma_display->setTextSize(2);
  dma_display->setTextColor(textColor);

  // Centrar el texto
  int textW = strlen(c.name) * 12;   // 6px * textSize(2)
  int textH = 16;                     // 8px * textSize(2)
  int x = (PANEL_RES_X * PANEL_CHAIN - textW) / 2;
  int y = (PANEL_RES_Y - textH) / 2;

  dma_display->setCursor(x, y);
  dma_display->print(c.name);

  dma_display->flipDMABuffer();

  Serial.printf("[%d/%d] %s  RGB(%3d,%3d,%3d)  brillo=%d\n",
                idx + 1, NUM_COLORS, c.name, c.r, c.g, c.b, panelBrightness);
}

// ─── Dibuja barras de color ─────────────────────────────
void showColorBars() {
  dma_display->fillScreen(0);

  int w = PANEL_RES_X * PANEL_CHAIN;  // 128
  int h = PANEL_RES_Y;                // 32
  int barW = w / 7;                   // ~18 px por barra

  struct { uint8_t r, g, b; } bars[7] = {
    {255, 0, 0},   // rojo
    {0, 255, 0},   // verde
    {0, 0, 255},   // azul
    {255, 255, 0}, // amarillo
    {0, 255, 255}, // cian
    {255, 0, 255}, // magenta
    {128, 128, 128},// gris
  };

  for (int i = 0; i < 7; i++) {
    dma_display->fillRect(i * barW, 0, barW - 1, h,
      dma_display->color565(bars[i].r, bars[i].g, bars[i].b));
  }

  dma_display->flipDMABuffer();
  Serial.println("Barras de color (7 bandas)");
}

// ─── Dibuja rampa de grises ─────────────────────────────
void showGrayRamp() {
  dma_display->fillScreen(0);
  int w = PANEL_RES_X * PANEL_CHAIN;
  int h = PANEL_RES_Y;

  for (int x = 0; x < w; x++) {
    uint8_t v = (x * 255) / (w - 1);
    dma_display->drawFastVLine(x, 0, h, dma_display->color565(v, v, v));
  }
  dma_display->flipDMABuffer();
  Serial.println("Rampa de grises (0-255)");
}

// ─── Ayuda serial ───────────────────────────────────────
void printHelp() {
  Serial.println("\n── COMANDOS ──────────────────────");
  Serial.println("  n     → color siguiente");
  Serial.println("  p     → color anterior");
  Serial.println("  a     → modo automático ON/OFF");
  Serial.println("  + / = → sube brillo (+5)");
  Serial.println("  -     → baja brillo (-5)");
  Serial.println("  b     → barras de color");
  Serial.println("  g     → rampa de grises");
  Serial.println("  h     → esta ayuda");
  Serial.println("  NUM   → ir al color N (0-9)");
  Serial.println("────────────────────────────────\n");
}

// ─── SETUP ──────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== CALIBRACIÓN DE COLOR HUB75 ===");

  initDisplay();
  dma_display->setPanelBrightness(panelBrightness);

  printHelp();
  showColor(currentColor);
  lastChange = millis();
}

// ─── LOOP ───────────────────────────────────────────────
void loop() {
  // ── Modo automático: cambia cada COLOR_DURATION_MS ──
  if (autoMode && (millis() - lastChange >= COLOR_DURATION_MS)) {
    currentColor = (currentColor + 1) % NUM_COLORS;
    showColor(currentColor);
    lastChange = millis();
  }

  // ── Comandos por serial ──────────────────────────────
  if (Serial.available()) {
    char c = Serial.read();
    autoMode = false;  // salir de modo auto al recibir comando

    switch (c) {
      case 'n':
        currentColor = (currentColor + 1) % NUM_COLORS;
        showColor(currentColor);
        break;
      case 'p':
        currentColor = (currentColor - 1 + NUM_COLORS) % NUM_COLORS;
        showColor(currentColor);
        break;
      case 'a':
        autoMode = !autoMode;
        Serial.printf("Modo automático: %s\n", autoMode ? "ON" : "OFF");
        if (autoMode) lastChange = millis();
        break;
      case '+': case '=':
        if (panelBrightness < 250) panelBrightness += 5;
        else panelBrightness = 255;
        dma_display->setPanelBrightness(panelBrightness);
        Serial.printf("Brillo: %d\n", panelBrightness);
        break;
      case '-':
        if (panelBrightness > 5) panelBrightness -= 5;
        else panelBrightness = 0;
        dma_display->setPanelBrightness(panelBrightness);
        Serial.printf("Brillo: %d\n", panelBrightness);
        break;
      case 'b':
        showColorBars();
        break;
      case 'g':
        showGrayRamp();
        break;
      case 'h':
        printHelp();
        break;
      case '0'...'9':
        currentColor = c - '0';
        if (currentColor < NUM_COLORS) showColor(currentColor);
        break;
      case '\n': case '\r': break;  // ignorar
      default:
        Serial.printf("Comando '%c' no reconocido. Pulsa 'h' para ayuda.\n", c);
        break;
    }
    lastChange = millis();
  }
}
