idf_component_register(
    SRCS "main.cpp"
    PRIV_REQUIRES spi_flash motores DualCore 
    REQUIRES app_wifi
    INCLUDE_DIRS "."
)
