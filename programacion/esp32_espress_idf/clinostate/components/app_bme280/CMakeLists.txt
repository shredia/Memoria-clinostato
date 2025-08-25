idf_component_register(
    SRCS "app_bme280.cpp"
    INCLUDE_DIRS "include"
    REQUIRES driver bme280 i2c_bus esp_timer mqtt app_mqtt
)