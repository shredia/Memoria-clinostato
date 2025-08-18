idf_component_register(
    SRCS "app_mqtt.cpp" 
    INCLUDE_DIRS "include" 
    REQUIRES mqtt esp_wifi esp_netif nvs_flash driver motores LP8 AccelStepper 
)