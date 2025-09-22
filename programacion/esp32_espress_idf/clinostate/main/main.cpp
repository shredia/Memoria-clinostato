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

#include "driver/uart.h"

#define UART_NUM1 UART_NUM_1
#define TXD_PIN1 (GPIO_NUM_17)
#define RXD_PIN1 (GPIO_NUM_16)

LP8 *sensor1 = nullptr;
LP8 *sensor2 = nullptr;


void motores_task(void *pvParameters) {
    ESP_LOGI("MOTORES", "Núcleo actual: %d", xPortGetCoreID());
    TickType_t last = xTaskGetTickCount();

    //recordar configurar el MenuConfig -> Config_FREERTOS_HZ de 100 a 1000
    const TickType_t period = pdMS_TO_TICKS(1); // 1–5 ms; prueba 1 ms

    for (;;) {
        actualizarMotores();      // Trabajo corto y NO bloqueante
        vTaskDelayUntil(&last, period);  // Cede SIEMPRE el CPU
    }
}
    

void comunicaciones_task(void *pvParameters) {
    mqtt_start("mqtt://192.168.31.81"); //ip oficina mqtt://192.168.31.81 ip casa: mqtt://192.168.1.8
    vTaskDelete(NULL);
}

extern "C" void app_main(void) {
    ESP_ERROR_CHECK(wifi_init());


    //creamos el objeto de LP8
    sensor1 = new LP8(GPIO_NUM_5,GPIO_NUM_4,UART_NUM_2);
    sensor1->Setup();

    sensor2 = new LP8(GPIO_NUM_19,GPIO_NUM_13,UART_NUM_1);
    sensor2->Setup();


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

 xTaskCreatePinnedToCore(
    [](void *pvParameters){ ((LP8*)pvParameters)->task_medicion_continua(pvParameters); },
    "MedicionLP8_2",
    4096,
    sensor2,
    5,
    NULL,
    0
);
    
    esp_err_t err = wifi_connect("Oficina AG", "OficinaAG23", 15000); //ID oficina:  Oficina AG ID casa: Renatita
    if (err == ESP_OK) {
        ESP_LOGI("MAIN", "¡Wi-Fi ok!");
        xTaskCreatePinnedToCore(comunicaciones_task, "Comunicaciones", 4096, NULL, 5, NULL, 0);
        bme280_start_periodic();
        xTaskCreatePinnedToCore(motores_task, "Motores", 4096, NULL, 5, NULL, 1);
    } else {
        ESP_LOGE("MAIN", "No se logró conectar");
    }

    
    

}





