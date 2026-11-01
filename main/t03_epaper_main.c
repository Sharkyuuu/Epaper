#include <stdlib.h>
#include <string.h>

#include "epaper_4in2_v2.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "t03_screen";

#define CHECK_EPAPER(operation, framebuffer)                                      \
    do {                                                                          \
        const esp_err_t result = (operation);                                      \
        if (result != ESP_OK) {                                                    \
            ESP_LOGE(TAG, "%s failed: %s; test stopped without rebooting",       \
                     #operation, esp_err_to_name(result));                         \
            free(framebuffer);                                                     \
            return;                                                               \
        }                                                                          \
    } while (0)

typedef struct {
    char character;
    uint8_t columns[5];
} glyph_t;

static const glyph_t FONT[] = {
    {'A', {0x7E, 0x09, 0x09, 0x09, 0x7E}},
    {'E', {0x7F, 0x49, 0x49, 0x49, 0x41}},
    {'K', {0x7F, 0x08, 0x14, 0x22, 0x41}},
    {'O', {0x3E, 0x41, 0x41, 0x41, 0x3E}},
    {'P', {0x7F, 0x09, 0x09, 0x09, 0x06}},
    {'R', {0x7F, 0x09, 0x19, 0x29, 0x46}},
};

static void set_black_pixel(uint8_t *framebuffer, int x, int y)
{
    if (x < 0 || x >= EPD_WIDTH || y < 0 || y >= EPD_HEIGHT) {
        return;
    }
    framebuffer[y * (EPD_WIDTH / 8) + x / 8] &= (uint8_t)~(0x80U >> (x % 8));
}

static void fill_rectangle(uint8_t *framebuffer, int x, int y, int width, int height)
{
    for (int py = y; py < y + height; py++) {
        for (int px = x; px < x + width; px++) {
            set_black_pixel(framebuffer, px, py);
        }
    }
}

static const uint8_t *find_glyph(char character)
{
    for (size_t i = 0; i < sizeof(FONT) / sizeof(FONT[0]); i++) {
        if (FONT[i].character == character) {
            return FONT[i].columns;
        }
    }
    return NULL;
}

static void draw_text(uint8_t *framebuffer, int x, int y, const char *text, int scale)
{
    for (const char *current = text; *current != '\0'; current++) {
        if (*current == ' ') {
            x += 4 * scale;
            continue;
        }

        const uint8_t *glyph = find_glyph(*current);
        if (glyph == NULL) {
            x += 6 * scale;
            continue;
        }

        for (int column = 0; column < 5; column++) {
            for (int row = 0; row < 7; row++) {
                if ((glyph[column] & (1U << row)) != 0) {
                    fill_rectangle(framebuffer,
                                   x + column * scale,
                                   y + row * scale,
                                   scale,
                                   scale);
                }
            }
        }
        x += 6 * scale;
    }
}

static void draw_test_pattern(uint8_t *framebuffer)
{
    memset(framebuffer, 0xFF, EPD_BUFFER_SIZE);

    fill_rectangle(framebuffer, 8, 8, EPD_WIDTH - 16, 4);
    fill_rectangle(framebuffer, 8, EPD_HEIGHT - 12, EPD_WIDTH - 16, 4);
    fill_rectangle(framebuffer, 8, 8, 4, EPD_HEIGHT - 16);
    fill_rectangle(framebuffer, EPD_WIDTH - 12, 8, 4, EPD_HEIGHT - 16);
    draw_text(framebuffer, 92, 118, "EPAPER OK", 5);
}

void app_main(void)
{
    uint8_t *framebuffer = heap_caps_malloc(EPD_BUFFER_SIZE, MALLOC_CAP_DMA);
    if (framebuffer == NULL) {
        ESP_LOGE(TAG, "failed to allocate %d-byte framebuffer", EPD_BUFFER_SIZE);
        return;
    }

    ESP_LOGI(TAG, "T03 screen test starting");
    CHECK_EPAPER(epaper_init(), framebuffer);

    ESP_LOGI(TAG, "showing white screen");
    memset(framebuffer, 0xFF, EPD_BUFFER_SIZE);
    CHECK_EPAPER(epaper_display(framebuffer), framebuffer);
    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_LOGI(TAG, "showing black screen");
    memset(framebuffer, 0x00, EPD_BUFFER_SIZE);
    CHECK_EPAPER(epaper_display(framebuffer), framebuffer);
    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_LOGI(TAG, "showing final border and text pattern");
    draw_test_pattern(framebuffer);
    CHECK_EPAPER(epaper_display(framebuffer), framebuffer);
    CHECK_EPAPER(epaper_sleep(), framebuffer);

    free(framebuffer);
    ESP_LOGI(TAG, "T03 screen test PASS; EPAPER OK should remain visible");
}
