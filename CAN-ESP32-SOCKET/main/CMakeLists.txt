idf_component_register(SRCS "can_2_mqtt.c" "app_main.c" "mcp2515_driver.c" "wifi_stuff.c"
                       PRIV_REQUIRES spi_flash driver
                       INCLUDE_DIRS ""
                       REQUIRES mcp2515 lwip esp_netif esp_wifi nvs_flash esp_driver_twai esp_timer
                       INCLUDE_DIRS "."
                    )
