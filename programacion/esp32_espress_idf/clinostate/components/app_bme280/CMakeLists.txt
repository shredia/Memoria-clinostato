idf_component_register(
    SRCS "app_bme280.cpp"
    INCLUDE_DIRS "include"
    REQUIRES i2c_bus bme280 esp_timer mqtt app_mqtt
)