idf_component_register(SRCS "t03_epaper_main.c" "epaper_4in2_v2.c"
                       PRIV_REQUIRES esp_driver_gpio esp_driver_spi
                       INCLUDE_DIRS ".")
