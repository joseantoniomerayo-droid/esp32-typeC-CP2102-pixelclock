#include <Arduino.h>
#include "display.h"
#include "qr_display.h"

// QR 29x29 para "WIFI:T:nopass;S:PixelClock-AP;;"
static const uint8_t qr_ap[29][4] = {
  {254,106,115,248}, {130,180,122,8}, {186,49,58,232}, {186,116,178,232},
  {186,172,226,232}, {130,91,178,8}, {254,170,171,248}, {0,41,120,0},
  {170,112,216,144}, {20,245,202,112}, {170,227,227,120}, {184,198,251,88},
  {162,219,75,88}, {192,131,10,112}, {11,4,105,152}, {205,161,115,0},
  {223,72,246,96}, {84,21,136,16}, {158,11,130,184}, {84,174,243,72},
  {171,195,159,216}, {0,179,88,224}, {254,92,58,152}, {130,64,104,136},
  {186,153,15,144}, {186,108,116,160}, {186,208,45,136}, {130,116,70,144},
  {254,219,121,24},
};

void showQRCode() {
  dma_display->fillScreen(0);

  // QR a la izquierda (blanco sobre negro)
  int qrX = 2, qrY = 2;
  for (int row = 0; row < 29; row++) {
    for (int col = 0; col < 29; col++) {
      int byte_idx = col / 8;
      int bit_idx = 7 - (col % 8);
      bool on = qr_ap[row][byte_idx] & (1 << bit_idx);
      uint16_t c = on ? dma_display->color565(255,255,255) : 0;
      dma_display->drawPixel(qrX + col, qrY + row, c);
    }
  }

  // Texto a la derecha
  int tx = qrX + 29 + 4;  // después del QR + margen
  dma_display->setTextSize(1);
  dma_display->setTextColor(dma_display->color565(100, 180, 255));
  dma_display->setCursor(tx, 2);
  dma_display->print("1. Conectate a");
  dma_display->setCursor(tx, 11);
  dma_display->setTextColor(dma_display->color565(255, 200, 0));
  dma_display->print("PixelClock-AP");
  dma_display->setCursor(tx, 20);
  dma_display->setTextColor(dma_display->color565(100, 255, 100));
  dma_display->print("2. Abre 192.168.4.1");

  dma_display->flipDMABuffer();
}
