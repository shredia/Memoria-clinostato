// main.cpp
#include <stdio.h>
#include "motores.h"
#include "Task_Core_0.h"
#include "Task_Core_1.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" void app_main(void) {
    printf("Iniciando aplicación\n");
    motores_setup();
    xTaskCreatePinnedToCore(Task_Core_0, "Core0", 2048, NULL, 1, NULL, 0); // núcleo 0
    xTaskCreatePinnedToCore(Task_Core_1, "Core1", 2048, NULL, 1, NULL, 1); // núcleo 1
}
