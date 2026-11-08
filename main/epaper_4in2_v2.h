#pragma once

#include <stdint.h>

#include "esp_err.h"

#define EPD_WIDTH 400
#define EPD_HEIGHT 300
#define EPD_BUFFER_SIZE (EPD_WIDTH * EPD_HEIGHT / 8)

//-------------------------------------------------------------------------------------------------------------------
// @brief      Initialize the Waveshare 4.2-inch V2 e-paper module.
// @return     ESP_OK on success, otherwise an ESP-IDF error code.
//-------------------------------------------------------------------------------------------------------------------
esp_err_t epaper_init(void);

//-------------------------------------------------------------------------------------------------------------------
// @brief      Transfer a complete monochrome frame and refresh the display.
// @param      framebuffer    15000-byte buffer, one bit per pixel.
// @return     ESP_OK on success, otherwise an ESP-IDF error code.
//-------------------------------------------------------------------------------------------------------------------
esp_err_t epaper_display(const uint8_t *framebuffer);

//-------------------------------------------------------------------------------------------------------------------
// @brief      Put the e-paper controller into deep sleep.
// @return     ESP_OK on success, otherwise an ESP-IDF error code.
//-------------------------------------------------------------------------------------------------------------------
esp_err_t epaper_sleep(void);
