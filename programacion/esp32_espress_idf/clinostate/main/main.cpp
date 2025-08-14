// main.cpp
#include <stdio.h>
#include "motores.h"
#include "Task_Core_0.h"
#include "Task_Core_1.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wifi.h"
#include "esp_log.h"


extern "C" void app_main(void) {
    
    ESP_ERROR_CHECK(wifi_init());

    esp_err_t err = wifi_connect("Oficina AG", "OficinaAG23", 15000);
    if (err == ESP_OK) {
        ESP_LOGI("MAIN", "¡Wi-Fi ok!");
        // aquí ya puedes iniciar sockets, MQTT, etc.
    } else {
        ESP_LOGE("MAIN", "No se logró conectar");
    }
    printf("Iniciando aplicación\n");
    motores_setup();
    xTaskCreatePinnedToCore(Task_Core_0, "Core0", 2048, NULL, 1, NULL, 0); // núcleo 0
    xTaskCreatePinnedToCore(Task_Core_1, "Core1", 2048, NULL, 1, NULL, 1); // núcleo 1
}
