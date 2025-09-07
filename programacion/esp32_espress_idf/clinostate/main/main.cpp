// main.cpp
#include <stdio.h>
#include "motores.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wifi.h"
#include "esp_log.h"
#include "app_mqtt.h"
#include "LP8_protocolo.hpp"
#include "LP8_libreria.hpp"
#include "app_bme280.hpp"


void motores_task(void *pvParameters) {
    ESP_LOGI("MOTORES", "Núcleo actual: %d", xPortGetCoreID());
    for(;;) {
        actualizarMotores();
         vTaskDelay(pdMS_TO_TICKS(10)); // En el pc no funciona 1 ms, se cambió a 5 ms
    }
}
    

void comunicaciones_task(void *pvParameters) {
    mqtt_start("mqtt://192.168.1.8"); //ip oficina mqtt://192.168.31.81 ip casa: mqtt://192.168.1.8
    vTaskDelete(NULL);
}

extern "C" void app_main(void) {
    ESP_ERROR_CHECK(wifi_init());


    //creamos el objeto de LP8
    LP8 *sensor1 = new LP8();
    sensor1->Setup();
    bme280_setup();
    motores_setup();
    xTaskCreatePinnedToCore(
    [](void *pvParameters){ ((LP8*)pvParameters)->task_medicion_continua(pvParameters); },
    "MedicionLP8_1",
    4096,
    sensor1,
    5,
    NULL,
    0
);
    
    esp_err_t err = wifi_connect("Renatita", "dino$auri0", 15000); //ID oficina:  Oficina AG ID casa: Renatita
    if (err == ESP_OK) {
        ESP_LOGI("MAIN", "¡Wi-Fi ok!");
        xTaskCreatePinnedToCore(comunicaciones_task, "Comunicaciones", 4096, NULL, 5, NULL, 0);
        bme280_start_periodic();
        xTaskCreatePinnedToCore(motores_task, "Motores", 4096, NULL, 5, NULL, 1);
    } else {
        ESP_LOGE("MAIN", "No se logró conectar");
    }

    
    

}





