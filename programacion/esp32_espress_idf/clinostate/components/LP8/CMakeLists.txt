idf_component_register(
    SRCS "src/LP8_libreria.cpp" "src/LP8_protocolo.cpp"
    INCLUDE_DIRS "include" 
    REQUIRES driver esp_timer mqtt app_mqtt
)