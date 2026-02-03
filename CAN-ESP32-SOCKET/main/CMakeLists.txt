idf_component_register(SRCS "app_main.c" "mcp2515_driver.c" "wifi_stuff.c"
                       PRIV_REQUIRES spi_flash driver
                       INCLUDE_DIRS ""
                       REQUIRES mcp2515 lwip esp_netif esp_wifi nvs_flash
                       INCLUDE_DIRS "."
                    )
