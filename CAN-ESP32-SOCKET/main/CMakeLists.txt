idf_component_register(SRCS "app_main.c" "can_stuff.c" "wifi_stuff.c"
                       PRIV_REQUIRES spi_flash driver
                       INCLUDE_DIRS ""
                       REQUIRES mcp2515)
