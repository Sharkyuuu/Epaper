#pragma once

#include <stdint.h>

#include "esp_err.h"

#define EPD_WIDTH 400
#define EPD_HEIGHT 300
#define EPD_BUFFER_SIZE (EPD_WIDTH * EPD_HEIGHT / 8)

esp_err_t epaper_init(void);
esp_err_t epaper_display(const uint8_t *framebuffer);
esp_err_t epaper_sleep(void);
