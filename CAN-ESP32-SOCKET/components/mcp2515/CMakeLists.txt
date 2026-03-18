idf_component_register(SRCS "mcp2515.c"
                        PRIV_REQUIRES spi_flash driver
                        INCLUDE_DIRS "."
                                        )
