# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "C:/Espressif/frameworks/esp-idf-v5.4/components/bootloader/subproject")
  file(MAKE_DIRECTORY "C:/Espressif/frameworks/esp-idf-v5.4/components/bootloader/subproject")
endif()
file(MAKE_DIRECTORY
<<<<<<< Updated upstream
<<<<<<< Updated upstream
  "D:/Utal/Memoria_Clinostato/Memoria-clinostato/programacion/esp32_espress_idf/clinostate/build/bootloader"
  "D:/Utal/Memoria_Clinostato/Memoria-clinostato/programacion/esp32_espress_idf/clinostate/build/bootloader-prefix"
  "D:/Utal/Memoria_Clinostato/Memoria-clinostato/programacion/esp32_espress_idf/clinostate/build/bootloader-prefix/tmp"
  "D:/Utal/Memoria_Clinostato/Memoria-clinostato/programacion/esp32_espress_idf/clinostate/build/bootloader-prefix/src/bootloader-stamp"
  "D:/Utal/Memoria_Clinostato/Memoria-clinostato/programacion/esp32_espress_idf/clinostate/build/bootloader-prefix/src"
  "D:/Utal/Memoria_Clinostato/Memoria-clinostato/programacion/esp32_espress_idf/clinostate/build/bootloader-prefix/src/bootloader-stamp"
=======
=======
>>>>>>> Stashed changes
  "D:/Damian/Utal/Memoria_clino/Memoria-clinostato/programacion/esp32_espress_idf/clinostate/build/bootloader"
  "D:/Damian/Utal/Memoria_clino/Memoria-clinostato/programacion/esp32_espress_idf/clinostate/build/bootloader-prefix"
  "D:/Damian/Utal/Memoria_clino/Memoria-clinostato/programacion/esp32_espress_idf/clinostate/build/bootloader-prefix/tmp"
  "D:/Damian/Utal/Memoria_clino/Memoria-clinostato/programacion/esp32_espress_idf/clinostate/build/bootloader-prefix/src/bootloader-stamp"
  "D:/Damian/Utal/Memoria_clino/Memoria-clinostato/programacion/esp32_espress_idf/clinostate/build/bootloader-prefix/src"
  "D:/Damian/Utal/Memoria_clino/Memoria-clinostato/programacion/esp32_espress_idf/clinostate/build/bootloader-prefix/src/bootloader-stamp"
<<<<<<< Updated upstream
>>>>>>> Stashed changes
=======
>>>>>>> Stashed changes
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
<<<<<<< Updated upstream
<<<<<<< Updated upstream
    file(MAKE_DIRECTORY "D:/Utal/Memoria_Clinostato/Memoria-clinostato/programacion/esp32_espress_idf/clinostate/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "D:/Utal/Memoria_Clinostato/Memoria-clinostato/programacion/esp32_espress_idf/clinostate/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
=======
=======
>>>>>>> Stashed changes
    file(MAKE_DIRECTORY "D:/Damian/Utal/Memoria_clino/Memoria-clinostato/programacion/esp32_espress_idf/clinostate/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "D:/Damian/Utal/Memoria_clino/Memoria-clinostato/programacion/esp32_espress_idf/clinostate/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
<<<<<<< Updated upstream
>>>>>>> Stashed changes
=======
>>>>>>> Stashed changes
endif()
