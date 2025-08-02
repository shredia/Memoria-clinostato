idf_component_register(SRCS "src/LP8_protocolo.c" "src/LP8_libreria.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES driver esp_timer)

                       
set_source_files_properties(src/LP8_libreria.cpp PROPERTIES LANGUAGE CXX)