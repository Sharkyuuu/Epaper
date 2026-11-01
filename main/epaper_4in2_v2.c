#include "epaper_4in2_v2.h"

#include <stdbool.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define EPD_PIN_BUSY GPIO_NUM_4
#define EPD_PIN_RST  GPIO_NUM_5
#define EPD_PIN_DC   GPIO_NUM_6
#define EPD_PIN_CS   GPIO_NUM_7
#define EPD_PIN_DIN  GPIO_NUM_11
#define EPD_PIN_CLK  GPIO_NUM_12

#define EPD_SPI_HOST SPI2_HOST
#define EPD_SPI_HZ   (2 * 1000 * 1000)
#define EPD_BUSY_TIMEOUT_MS 60000

static const char *TAG = "epaper";
static spi_device_handle_t s_spi;

static esp_err_t epaper_spi_write(const uint8_t *data, size_t length)
{
    spi_transaction_t transaction = {
        .length = length * 8,
    };

    if (length <= sizeof(transaction.tx_data)) {
        transaction.flags = SPI_TRANS_USE_TXDATA;
        memcpy(transaction.tx_data, data, length);
    } else {
        transaction.tx_buffer = data;
    }
    return spi_device_transmit(s_spi, &transaction);
}

static esp_err_t epaper_send_command(uint8_t command)
{
    gpio_set_level(EPD_PIN_DC, 0);
    return epaper_spi_write(&command, 1);
}

static esp_err_t epaper_send_data(const uint8_t *data, size_t length)
{
    gpio_set_level(EPD_PIN_DC, 1);
    return epaper_spi_write(data, length);
}

static esp_err_t epaper_send_byte(uint8_t data)
{
    return epaper_send_data(&data, 1);
}

static esp_err_t epaper_wait_idle(const char *stage)
{
    const TickType_t start = xTaskGetTickCount();
    const TickType_t timeout = pdMS_TO_TICKS(EPD_BUSY_TIMEOUT_MS);

    while (gpio_get_level(EPD_PIN_BUSY) == 1) {
        if ((xTaskGetTickCount() - start) > timeout) {
            ESP_LOGE(TAG, "BUSY timeout during %s; check BUSY/RST and BS=0", stage);
            return ESP_ERR_TIMEOUT;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    return ESP_OK;
}

static void epaper_hardware_reset(void)
{
    gpio_set_level(EPD_PIN_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(EPD_PIN_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(2));
    gpio_set_level(EPD_PIN_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
}

static esp_err_t epaper_refresh(void)
{
    ESP_RETURN_ON_ERROR(epaper_send_command(0x22), TAG, "display update command failed");
    ESP_RETURN_ON_ERROR(epaper_send_byte(0xF7), TAG, "display update data failed");
    ESP_RETURN_ON_ERROR(epaper_send_command(0x20), TAG, "display activation failed");
    return epaper_wait_idle("display refresh");
}

esp_err_t epaper_init(void)
{
    const gpio_config_t output_config = {
        .pin_bit_mask = (1ULL << EPD_PIN_RST) | (1ULL << EPD_PIN_DC),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&output_config), TAG, "output GPIO setup failed");

    const gpio_config_t input_config = {
        .pin_bit_mask = 1ULL << EPD_PIN_BUSY,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&input_config), TAG, "BUSY GPIO setup failed");

    const spi_bus_config_t bus_config = {
        .mosi_io_num = EPD_PIN_DIN,
        .miso_io_num = GPIO_NUM_NC,
        .sclk_io_num = EPD_PIN_CLK,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = EPD_BUFFER_SIZE,
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(EPD_SPI_HOST, &bus_config, SPI_DMA_CH_AUTO),
                        TAG, "SPI bus setup failed");

    const spi_device_interface_config_t device_config = {
        .clock_speed_hz = EPD_SPI_HZ,
        .mode = 0,
        .spics_io_num = EPD_PIN_CS,
        .queue_size = 1,
    };
    ESP_RETURN_ON_ERROR(spi_bus_add_device(EPD_SPI_HOST, &device_config, &s_spi),
                        TAG, "SPI device setup failed");

    epaper_hardware_reset();
    ESP_RETURN_ON_ERROR(epaper_wait_idle("hardware reset"), TAG, "display stayed busy");
    ESP_RETURN_ON_ERROR(epaper_send_command(0x12), TAG, "software reset failed");
    ESP_RETURN_ON_ERROR(epaper_wait_idle("software reset"), TAG, "display stayed busy");

    ESP_RETURN_ON_ERROR(epaper_send_command(0x21), TAG, "display update control failed");
    const uint8_t update_control[] = {0x40, 0x00};
    ESP_RETURN_ON_ERROR(epaper_send_data(update_control, sizeof(update_control)), TAG,
                        "display update control data failed");

    ESP_RETURN_ON_ERROR(epaper_send_command(0x3C), TAG, "border waveform command failed");
    ESP_RETURN_ON_ERROR(epaper_send_byte(0x05), TAG, "border waveform data failed");
    ESP_RETURN_ON_ERROR(epaper_send_command(0x11), TAG, "data entry command failed");
    ESP_RETURN_ON_ERROR(epaper_send_byte(0x03), TAG, "data entry data failed");

    ESP_RETURN_ON_ERROR(epaper_send_command(0x44), TAG, "RAM X window command failed");
    const uint8_t x_window[] = {0x00, 0x31};
    ESP_RETURN_ON_ERROR(epaper_send_data(x_window, sizeof(x_window)), TAG,
                        "RAM X window data failed");

    ESP_RETURN_ON_ERROR(epaper_send_command(0x45), TAG, "RAM Y window command failed");
    const uint8_t y_window[] = {0x00, 0x00, 0x2B, 0x01};
    ESP_RETURN_ON_ERROR(epaper_send_data(y_window, sizeof(y_window)), TAG,
                        "RAM Y window data failed");

    ESP_RETURN_ON_ERROR(epaper_send_command(0x4E), TAG, "RAM X cursor command failed");
    ESP_RETURN_ON_ERROR(epaper_send_byte(0x00), TAG, "RAM X cursor data failed");
    ESP_RETURN_ON_ERROR(epaper_send_command(0x4F), TAG, "RAM Y cursor command failed");
    const uint8_t y_cursor[] = {0x00, 0x00};
    ESP_RETURN_ON_ERROR(epaper_send_data(y_cursor, sizeof(y_cursor)), TAG,
                        "RAM Y cursor data failed");
    ESP_RETURN_ON_ERROR(epaper_wait_idle("initialization"), TAG, "display stayed busy");

    ESP_LOGI(TAG, "4.2-inch V2 initialized");
    return ESP_OK;
}

esp_err_t epaper_display(const uint8_t *framebuffer)
{
    if (framebuffer == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_RETURN_ON_ERROR(epaper_send_command(0x24), TAG, "old image command failed");
    ESP_RETURN_ON_ERROR(epaper_send_data(framebuffer, EPD_BUFFER_SIZE), TAG,
                        "old image transfer failed");
    ESP_RETURN_ON_ERROR(epaper_send_command(0x26), TAG, "new image command failed");
    ESP_RETURN_ON_ERROR(epaper_send_data(framebuffer, EPD_BUFFER_SIZE), TAG,
                        "new image transfer failed");
    return epaper_refresh();
}

esp_err_t epaper_sleep(void)
{
    ESP_RETURN_ON_ERROR(epaper_send_command(0x10), TAG, "sleep command failed");
    ESP_RETURN_ON_ERROR(epaper_send_byte(0x01), TAG, "sleep data failed");
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_LOGI(TAG, "display entered deep sleep");
    return ESP_OK;
}
