idf_component_register(SRCS "LCD_Display.c"
                            "Pixel_Array_Presets.c"
                       PRIV_REQUIRES spi_flash
                       REQUIRES driver
                       REQUIRES esp_lcd
                       INCLUDE_DIRS "")
