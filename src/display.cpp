#include <Arduino.h>
#include "display.h"

MatrixPanel_I2S_DMA *dma_display = nullptr;

void initDisplay() {
  HUB75_I2S_CFG::i2s_pins _pins = {
    PIN_R1, PIN_G1, PIN_B1, PIN_R2, PIN_G2, PIN_B2,
    PIN_A, PIN_B, PIN_C, PIN_D, PIN_E,
    PIN_LAT, PIN_OE, PIN_CLK
  };

  HUB75_I2S_CFG mxConfig(PANEL_RES_X, PANEL_RES_Y, PANEL_CHAIN, _pins);
  mxConfig.clkphase = false;
  mxConfig.i2sspeed = HUB75_I2S_CFG::HZ_10M;
  mxConfig.double_buff = true;

  dma_display = new MatrixPanel_I2S_DMA(mxConfig);
  dma_display->begin();
  dma_display->setTextWrap(false);
}
