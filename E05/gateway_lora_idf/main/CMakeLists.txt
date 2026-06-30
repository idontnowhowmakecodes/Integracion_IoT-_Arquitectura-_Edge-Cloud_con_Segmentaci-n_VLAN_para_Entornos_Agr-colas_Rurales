idf_component_register(SRCS "main.c" "lora.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_eth esp_netif esp_event nvs_flash driver esp_driver_gpio esp_driver_spi)
