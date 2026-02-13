idf_component_register(SRCS "app_main.c" "mcp2515_driver.c" "wifi_stuff.c" "twai_task.c"
                       PRIV_REQUIRES spi_flash driver
                       INCLUDE_DIRS ""
                       REQUIRES mcp2515 lwip esp_netif esp_wifi nvs_flash esp_driver_twai
                       INCLUDE_DIRS "."
                    )
