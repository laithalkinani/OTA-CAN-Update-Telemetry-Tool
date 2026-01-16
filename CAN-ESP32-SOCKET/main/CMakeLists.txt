idf_component_register(SRCS "ringbuffer.c" "core1.c" "core0.c" "app_main.c" "can_stuff.c" "wifi_stuff.c"
                       PRIV_REQUIRES spi_flash driver
                       INCLUDE_DIRS ""
                       REQUIRES mcp2515 lwip esp_netif esp_wifi nvs_flash
                       INCLUDE_DIRS "."
                    )
