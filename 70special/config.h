#pragma once

#define PICO_FLASH_SIZE_BYTES (512 * 1024)

// Display Configuration
#define ST7735_NUM_DEVICES 1

#define SPI_SCK_PIN GP18
#define SPI_MOSI_PIN GP19

#define TFT_CS_PIN GP3
#define TFT_DC_PIN GP4
#define TFT_RST_PIN GP5