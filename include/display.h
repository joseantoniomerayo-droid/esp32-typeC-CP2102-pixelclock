#pragma once
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

// Panel configuration
#define PANEL_RES_X 64
#define PANEL_RES_Y 32
#define PANEL_CHAIN 2

// Pin mapping for DMDos Board V3
#define PIN_R1 25
#define PIN_G1 26
#define PIN_B1 27
#define PIN_R2 14
#define PIN_G2 12
#define PIN_B2 13
#define PIN_A  33
#define PIN_B  32
#define PIN_C  22
#define PIN_D  17
#define PIN_E  -1
#define PIN_LAT 4
#define PIN_OE 15
#define PIN_CLK 16

extern MatrixPanel_I2S_DMA *dma_display;

void initDisplay();
