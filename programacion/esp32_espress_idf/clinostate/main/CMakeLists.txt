idf_component_register(
    SRCS "main.cpp"
    PRIV_REQUIRES spi_flash motores DualCore 
    REQUIRES app_wifi app_mqtt LP8 app_bme280
    INCLUDE_DIRS "."
)
