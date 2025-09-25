#include "LP8_libreria.hpp"
#include "LP8_protocolo.hpp"
#include "esp_timer.h"
#include "mqtt_client.h"
#include "app_mqtt.h"
#include "esp_log.h" // <-- Asegúrate de incluir esto

uint8_t write_partida[]   = {0xfe, 0x41, 0x00, 0x80, 0x01, 0x10, 0x28, 0x7e};
uint8_t write_normal[]    = {0xfe, 0x41, 0x00, 0x80, 0x01, 0x20, 0x28, 0x6A};
uint8_t write_calibrar[]  = {0xfe, 0x41, 0x00, 0x80, 0x01, 0x53, 0x69, 0x8F};
uint8_t read_32_bytes[]   = {0xfe, 0x44, 0x00, 0x80, 0x20, 0x79, 0x3C};
uint8_t read_44_bytes[]   = {0xfe, 0x44, 0x00, 0x80, 0x2c, 0x79, 0x39};
uint8_t read_4_bytes[]    = {0xfe, 0x44, 0x00, 0xA4, 0x4, 0x62, 0x27};
uint8_t write_normal_prev_data[33];

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
                if(sensor->GetFirst_Sense()) {
                    sensor->SetFirst_Sense(false);
                }
                sensor->publicar_datos_sensor();
            } else {
                ESP_LOGW("LP8", "[UART%d] Error en el CRC.", sensor->GetPort());
            }
            vTaskDelay(pdMS_TO_TICKS(sensor->GetTime_Sense()));  // Esperar 20 segundos entre mediciones
        } else {
            vTaskDelay(pdMS_TO_TICKS(100)); // Pequeño delay para no saturar el CPU
        }
    }
}

void LP8::medir() {
    gpio_set_level(_vbb_en1, 0); // LOW
    vTaskDelay(pdMS_TO_TICKS(1000));
    gpio_set_level(_vbb_en1, 1); // HIGH
    vTaskDelay(pdMS_TO_TICKS(1000));

    int64_t start = esp_timer_get_time();

    while (gpio_get_level(_rdy1) != 0 && (esp_timer_get_time() - start < GetTime_Out())) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    if (gpio_get_level(_rdy1) == 0) {
        ESP_LOGW("LP8", "[UART%d] rdy1 pasó a LOW correctamente", GetPort());
    } else {
        ESP_LOGW("LP8", "[UART%d] ⏱ Timeout esperando rdy1 en LOW", GetPort());
    }

    if (GetCalibrar()) {
        SendRequest(write_calibrar);
    } else if (GetFirst_Sense()) {
        SendRequest(write_partida);
    } else {
        SendRequest(write_normal);
    }

    start = esp_timer_get_time();

    while (gpio_get_level(_rdy1) != 1 && (esp_timer_get_time() - start <  GetTime_Out())) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    if (gpio_get_level(_rdy1) == 1) {
        ESP_LOGW("LP8", "[UART%d] rdy1 volvió a HIGH correctamente", GetPort());
    } else {
        ESP_LOGW("LP8", "[UART%d] ⏱ Timeout esperando rdy1 en HIGH", GetPort());
    }

    SendRequest(read_44_bytes);
    ProcesarLectura();
    Generate_Request();

    int uart_num = GetPort();
    ESP_LOGW("LP8", "[UART%d] uart sensor: %d", uart_num, uart_num);
    ESP_LOGW("LP8", "[UART%d] === Fin del ciclo de medición ===", uart_num);
}

void LP8::publicar_datos_sensor() {
    int uart_num = GetPort();

    // CO2
    char mensaje_co2[32];
    snprintf(mensaje_co2, sizeof(mensaje_co2), "%.2f", Get_C02());
    char topic_co2[64];
    snprintf(topic_co2, sizeof(topic_co2), "esp32/%s/LP8_%d/co2", client_id, uart_num);
    ESP_LOGW("LP8", "[UART%d] [MQTT] Publicando: topic=%s, mensaje=%s", uart_num, topic_co2, mensaje_co2);
    esp_mqtt_client_publish(client, topic_co2, mensaje_co2, 0, 1, 0);

    // Presión
    char mensaje_presion[32];
    snprintf(mensaje_presion, sizeof(mensaje_presion), "%.1f", GetPresion());
    char topic_presion[64];
    snprintf(topic_presion, sizeof(topic_presion), "esp32/%s/LP8_%d/presion", client_id, uart_num);
    ESP_LOGW("LP8", "[UART%d] [MQTT] Publicando: topic=%s, mensaje=%s", uart_num, topic_presion, mensaje_presion);
    esp_mqtt_client_publish(client, topic_presion, mensaje_presion, 0, 1, 0);

    // Vcap1
    char mensaje_vcap1[32];
    snprintf(mensaje_vcap1, sizeof(mensaje_vcap1), "%u", GetVcap1());
    char topic_vcap1[64];
    snprintf(topic_vcap1, sizeof(topic_vcap1), "esp32/%s/LP8_%d/vcap1", client_id, uart_num);
    ESP_LOGW("LP8", "[UART%d] [MQTT] Publicando: topic=%s, mensaje=%s", uart_num, topic_vcap1, mensaje_vcap1);
    esp_mqtt_client_publish(client, topic_vcap1, mensaje_vcap1, 0, 1, 0);

    // Vcap2
    char mensaje_vcap2[32];
    snprintf(mensaje_vcap2, sizeof(mensaje_vcap2), "%u", GetVcap2());
    char topic_vcap2[64];
    snprintf(topic_vcap2, sizeof(topic_vcap2), "esp32/%s/LP8_%d/vcap2", client_id, uart_num);
    ESP_LOGW("LP8", "[UART%d] [MQTT] Publicando: topic=%s, mensaje=%s", uart_num, topic_vcap2, mensaje_vcap2);
    esp_mqtt_client_publish(client, topic_vcap2, mensaje_vcap2, 0, 1, 0);

    char mensaje_temp[32];
    snprintf(mensaje_temp, sizeof(mensaje_temp), "%f", Gettemp());
    char topic_temp[64];
    snprintf(topic_temp, sizeof(topic_temp), "esp32/%s/LP8_%d/temp", client_id, uart_num);
    ESP_LOGW("LP8", "[UART%d] [MQTT] Publicando: topic=%s, mensaje=%s", uart_num, topic_temp, mensaje_temp);
    esp_mqtt_client_publish(client, topic_temp, mensaje_temp, 0, 1, 0);

    // Concatenar errores en un solo mensaje
    char mensaje_error[64];
    snprintf(
        mensaje_error, sizeof(mensaje_error),
        "[%u][%u][%u][%u]",
        GetError(3), GetError(2), GetError(1), GetError(0)
    );
    char topic_error[64];
    snprintf(topic_error, sizeof(topic_error), "esp32/%s/LP8_%d/errores", client_id, uart_num);
    ESP_LOGW("LP8", "[UART%d] [MQTT] Publicando errores: topic=%s, mensaje=%s", uart_num, topic_error, mensaje_error);
    esp_mqtt_client_publish(client, topic_error, mensaje_error, 0, 1, 0);
}

void LP8::Generate_Request() {
    // write_normal_prev_data debe ser un arreglo de al menos 8 bytes
    write_normal_prev_data[0] = 0xfe;
    write_normal_prev_data[1] = 0x41;
    write_normal_prev_data[2] = 0x00;
    write_normal_prev_data[3] = 0x80;
    write_normal_prev_data[4] = 0x01;    // Código de comando
    write_normal_prev_data[5] = 0x20;  // comando de escritura normal
    for(int i = 6; i < 23+6; i++) {
        write_normal_prev_data[i] = _sensor_state[i-6]; // Inicializa los parámetros a 0
    }
    write_normal_prev_data[23+6] = 0x27; // Presión High (temporalmente 27)
    write_normal_prev_data[24+6] = 0x8c; // Presión Low (temporalmente 8c)
    // Calcula CRC
    uint16_t crc = 0xFFFF;
  for (int pos = 0; pos < 25+6; pos++) {
    crc ^= (uint16_t)write_normal_prev_data[pos];
    for (int i = 0; i < 8; i++) {
      if (crc & 0x0001) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
    write_normal_prev_data[25+6] = (crc >> 8) & 0xFF; // CRC High
    write_normal_prev_data[26+6] = crc & 0xFF;        // CRC Low

}