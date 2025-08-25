#include "LP8_libreria.hpp"
#include "LP8_protocolo.hpp"
#include "esp_timer.h"
#include "mqtt_client.h"
#include "app_mqtt.h"    

uint8_t write_partida[]   = {0xfe, 0x41, 0x00, 0x80, 0x01, 0x10, 0x28, 0x7e};
uint8_t write_normal[]    = {0xfe, 0x41, 0x00, 0x80, 0x01, 0x20, 0x28, 0x6A};
uint8_t write_calibrar[]  = {0xfe, 0x41, 0x00, 0x80, 0x01, 0x53, 0x69, 0x8F};
uint8_t read_32_bytes[]   = {0xfe, 0x44, 0x00, 0x80, 0x20, 0x79, 0x3C};
uint8_t read_44_bytes[]   = {0xfe, 0x44, 0x00, 0x80, 0x2c, 0x79, 0x39};
uint8_t read_4_bytes[]    = {0xfe, 0x44, 0x00, 0xA4, 0x4, 0x62, 0x27};

// Supón que tienes acceso al cliente MQTT
extern esp_mqtt_client_handle_t mqtt_client;

void LP8::task_medicion_continua(void *pvParameters) {
    LP8 *sensor = (LP8 *)pvParameters;
    sensor->Setup();
    while (1) {
        if (continuousMeasurement) {
            sensor->Reset_receive(); // Resetea el buffer de recepción
            sensor->medir();

            // Verifica el flag CRC y publica si es true
            if (sensor->GetCrc_flag()) {
                // Publica CO2
                
                
                if(sensor->GetFirst_Sense()) {
                    sensor->SetFirst_Sense(false);
                }
                sensor->publicar_datos_sensor();
            }else{
                printf("Error en el CRC.\n");
            }
           


            vTaskDelay(pdMS_TO_TICKS(sensor->GetTime_Sense()));  // Esperar 20 segundos entre mediciones
        } else {
            vTaskDelay(pdMS_TO_TICKS(100)); // Pequeño delay para no saturar el CPU
        }
    }
}

void LP8::medir() {
    // Alimentación: vbb_en1 LOW -> HIGH con delays
    gpio_set_level(_vbb_en1, 0); // LOW
    vTaskDelay(pdMS_TO_TICKS(1000));
    gpio_set_level(_vbb_en1, 1); // HIGH
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Esperar a que rdy1 se ponga en LOW (timeout 500 ms)
    int64_t start = esp_timer_get_time();

    while (gpio_get_level(_rdy1) != 0 && (esp_timer_get_time() - start < GetTime_Out())) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    if (gpio_get_level(_rdy1) == 0) {

        printf("rdy1 pasó a LOW correctamente\n");
    } else {
        printf("⏱ Timeout esperando rdy1 en LOW\n");
    }

    // Enviar comando según estado
    if (GetCalibrar()) {
        SendRequest(write_calibrar);
    } else if (GetFirst_Sense()) {
        SendRequest(write_partida);
    } else {
        SendRequest(write_normal);
    }

    // Esperar a que rdy1 vuelva a HIGH (timeout 1000 ms)
    start = esp_timer_get_time();

    while (gpio_get_level(_rdy1) != 1 && (esp_timer_get_time() - start <  GetTime_Out())) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    if (gpio_get_level(_rdy1) == 1) {

        printf("rdy1 volvió a HIGH correctamente\n");
    } else {
        printf("⏱ Timeout esperando rdy1 en HIGH\n");
    }

    // Enviar read_44_bytes y procesar lectura
    SendRequest(read_44_bytes);
    ProcesarLectura(); // Procesamos la lectura y guardamos el flag crc si es correcto o no.

    
    // mostrar_co2(); // Implementa esta función según tu lógica

    printf("=== Fin del ciclo de medición ===\n\n");
}

void LP8::publicar_datos_sensor() {
    int uart_num = GetPort(); // Obtiene el número de UART del sensor

    // CO2
    char mensaje_co2[32];
    snprintf(mensaje_co2, sizeof(mensaje_co2), "CO2: %.2f", Get_C02());
    char topic_co2[64];
    snprintf(topic_co2, sizeof(topic_co2), "esp32/%s/LP8_%d/co2", client_id, uart_num);
    printf("[MQTT] Publicando: topic=%s, mensaje=%s\n", topic_co2, mensaje_co2);
    esp_mqtt_client_publish(client, topic_co2, mensaje_co2, 0, 1, 0);

    // Presión
    char mensaje_presion[32];
    snprintf(mensaje_presion, sizeof(mensaje_presion), "Presion: %.1f", GetPresion());
    char topic_presion[64];
    snprintf(topic_presion, sizeof(topic_presion), "esp32/%s/LP8_%d/presion", client_id, uart_num);
    printf("[MQTT] Publicando: topic=%s, mensaje=%s\n", topic_presion, mensaje_presion);
    esp_mqtt_client_publish(client, topic_presion, mensaje_presion, 0, 1, 0);

    // Vcap1
    char mensaje_vcap1[32];
    snprintf(mensaje_vcap1, sizeof(mensaje_vcap1), "Vcap1: %u", GetVcap1());
    char topic_vcap1[64];
    snprintf(topic_vcap1, sizeof(topic_vcap1), "esp32/%s/LP8_%d/vcap1", client_id, uart_num);
    printf("[MQTT] Publicando: topic=%s, mensaje=%s\n", topic_vcap1, mensaje_vcap1);
    esp_mqtt_client_publish(client, topic_vcap1, mensaje_vcap1, 0, 1, 0);

    // Vcap2
    char mensaje_vcap2[32];
    snprintf(mensaje_vcap2, sizeof(mensaje_vcap2), "Vcap2: %u", GetVcap2());
    char topic_vcap2[64];
    snprintf(topic_vcap2, sizeof(topic_vcap2), "esp32/%s/LP8_%d/vcap2", client_id, uart_num);
    printf("[MQTT] Publicando: topic=%s, mensaje=%s\n", topic_vcap2, mensaje_vcap2);
    esp_mqtt_client_publish(client, topic_vcap2, mensaje_vcap2, 0, 1, 0);

    // Errores
    for (int i = 0; i < 4; i++) {
        char mensaje_error[32];
        snprintf(mensaje_error, sizeof(mensaje_error), "Error%d: %u", i, GetError(i));
        char topic_error[64];
        snprintf(topic_error, sizeof(topic_error), "esp32/%s/LP8_%d/error%d", client_id, uart_num, i);
        esp_mqtt_client_publish(client, topic_error, mensaje_error, 0, 1, 0);
    }
    
}

