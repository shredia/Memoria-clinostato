cmake_minimum_required(VERSION 3.16)

# Añade subcarpetas donde están los componentes personalizados
set(EXTRA_COMPONENT_DIRS 
    ${CMAKE_SOURCE_DIR}/components/sensores/LP8
    #//${CMAKE_SOURCE_DIR}/components/comunicacion
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(clinostate)
