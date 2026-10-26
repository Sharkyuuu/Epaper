#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "esp_chip_info.h"
#include "esp_err.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_littlefs.h"
#include "esp_log.h"
#include "esp_psram.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

static const char *TAG = "t02_base";
static const char *TEST_FILE = "/littlefs/t02_test.txt";
static const char *TEST_TEXT = "T02 LittleFS read/write/delete test\n";

static void init_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

static void print_hardware_info(void)
{
    esp_chip_info_t chip_info;
    uint32_t flash_size = 0;

    esp_chip_info(&chip_info);
    ESP_ERROR_CHECK(esp_flash_get_size(NULL, &flash_size));

    const size_t psram_size = esp_psram_get_size();
    const size_t free_internal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    const size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    const size_t largest_internal = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);

    ESP_LOGI(TAG, "ESP-IDF version: %s", esp_get_idf_version());
    ESP_LOGI(TAG, "Chip: %s, cores: %u, revision: v%u.%u",
             CONFIG_IDF_TARGET,
             chip_info.cores,
             chip_info.revision / 100,
             chip_info.revision % 100);
    ESP_LOGI(TAG, "Flash: %" PRIu32 " bytes (%" PRIu32 " MB)",
             flash_size, flash_size / (1024U * 1024U));
    ESP_LOGI(TAG, "PSRAM: %u bytes (%u MB)",
             (unsigned)psram_size, (unsigned)(psram_size / (1024U * 1024U)));
    ESP_LOGI(TAG, "Free internal heap: %u bytes", (unsigned)free_internal);
    ESP_LOGI(TAG, "Largest internal block: %u bytes", (unsigned)largest_internal);
    ESP_LOGI(TAG, "Free PSRAM heap: %u bytes", (unsigned)free_psram);

    if (flash_size != 8U * 1024U * 1024U) {
        ESP_LOGE(TAG, "Unexpected Flash size; expected 8 MB");
    }
    if (psram_size != 8U * 1024U * 1024U) {
        ESP_LOGE(TAG, "Unexpected PSRAM size; expected 8 MB");
    }
}

static void run_littlefs_test(void)
{
    const esp_vfs_littlefs_conf_t conf = {
        .base_path = "/littlefs",
        .partition_label = "storage",
        .format_if_mount_failed = true,
        .dont_mount = false,
    };

    ESP_ERROR_CHECK(esp_vfs_littlefs_register(&conf));

    size_t total = 0;
    size_t used = 0;
    ESP_ERROR_CHECK(esp_littlefs_info(conf.partition_label, &total, &used));
    ESP_LOGI(TAG, "LittleFS mounted: total=%u bytes, used=%u bytes",
             (unsigned)total, (unsigned)used);

    FILE *file = fopen(TEST_FILE, "w");
    if (file == NULL) {
        ESP_LOGE(TAG, "Failed to open test file for writing: errno=%d", errno);
        ESP_ERROR_CHECK(ESP_FAIL);
    }

    const size_t expected_len = strlen(TEST_TEXT);
    const size_t written = fwrite(TEST_TEXT, 1, expected_len, file);
    if (fclose(file) != 0 || written != expected_len) {
        ESP_LOGE(TAG, "Failed to write test file completely");
        ESP_ERROR_CHECK(ESP_FAIL);
    }
    ESP_LOGI(TAG, "LittleFS write: OK");

    char buffer[64] = {0};
    file = fopen(TEST_FILE, "r");
    if (file == NULL) {
        ESP_LOGE(TAG, "Failed to open test file for reading: errno=%d", errno);
        ESP_ERROR_CHECK(ESP_FAIL);
    }

    const size_t read_len = fread(buffer, 1, sizeof(buffer) - 1, file);
    fclose(file);
    if (read_len != expected_len || memcmp(buffer, TEST_TEXT, expected_len) != 0) {
        ESP_LOGE(TAG, "LittleFS read-back comparison failed");
        ESP_ERROR_CHECK(ESP_FAIL);
    }
    ESP_LOGI(TAG, "LittleFS read and compare: OK");

    if (remove(TEST_FILE) != 0) {
        ESP_LOGE(TAG, "Failed to delete test file: errno=%d", errno);
        ESP_ERROR_CHECK(ESP_FAIL);
    }
    ESP_LOGI(TAG, "LittleFS delete: OK");
    ESP_LOGI(TAG, "T02 filesystem self-test: PASS");
}

void app_main(void)
{
    init_nvs();
    print_hardware_info();
    run_littlefs_test();
    ESP_LOGI(TAG, "T02 startup completed; device will remain running");
}
