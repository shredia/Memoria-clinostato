// main.cpp
#include <stdio.h>
#include "motores.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wifi.h"
#include "esp_log.h"
#include "app_mqtt.h"




void motores_task(void *pvParameters) {
    ESP_LOGI("MOTORES", "Núcleo actual: %d", xPortGetCoreID());

    

    for(;;) {
        actualizarMotores();

        
         vTaskDelay(pdMS_TO_TICKS(1)); // Espera 1 milisegundo
    }
}


void comunicaciones_task(void *pvParameters) {
    mqtt_start("mqtt://192.168.31.81");
    vTaskDelete(NULL);
}

extern "C" void app_main(void) {
    ESP_ERROR_CHECK(wifi_init());

    esp_err_t err = wifi_connect("Oficina AG", "OficinaAG23", 15000);
    if (err == ESP_OK) {
        ESP_LOGI("MAIN", "¡Wi-Fi ok!");
        xTaskCreatePinnedToCore(comunicaciones_task, "Comunicaciones", 4096, NULL, 5, NULL, 0);
    } else {
        ESP_LOGE("MAIN", "No se logró conectar");
    }

    motores_setup();
    xTaskCreatePinnedToCore(motores_task, "Motores", 4096, NULL, 5, NULL, 1);

}





