idf_component_register(
    SRCS "AccelStepper.cpp" "MultiStepper.cpp"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_timer
)
