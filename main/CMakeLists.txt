idf_component_register(SRCS "hello_world_main.c"
                       PRIV_REQUIRES esp_psram nvs_flash spi_flash
                       INCLUDE_DIRS ".")
