idf_component_register(
    SRCS "main.c"
    PRIV_REQUIRES spi_flash motores 
    INCLUDE_DIRS "."
)
