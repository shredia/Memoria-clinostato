#include "app_mqtt.h"
#include "esp_event.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_wifi.h" 
#include "esp_system.h"
#include "motores.h" // Asegúrate que este header tiene: extern AccelStepper MotorX; extern AccelStepper MotorY;
#include "LP8_libreria.hpp"
#include <driver/gpio.h>




static const char *TAG = "MQTT";
static esp_mqtt_client_handle_t client = NULL;

static char client_id[20]; // Global para usar en la tarea

static void mqtt_event_handler_cb(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    switch (event_id) {
        case MQTT_EVENT_CONNECTED: {
            ESP_LOGI(TAG, "Conectado a MQTT");
            char topic_motor[64], topic_sensor[64];
            snprintf(topic_motor, sizeof(topic_motor), "esp32/%s/motor/#", client_id);
            snprintf(topic_sensor, sizeof(topic_sensor), "esp32/%s/sensor/#", client_id);

            esp_mqtt_client_subscribe(client, topic_motor, 0);
            esp_mqtt_client_subscribe(client, topic_sensor, 0);
            break;
        }
        case MQTT_EVENT_DATA: {
            // 1) Copia segura del tópico a string C
            char topic_buf[96];
            int tlen = event->topic_len;
            if (tlen >= (int)sizeof(topic_buf)) tlen = sizeof(topic_buf) - 1;
            memcpy(topic_buf, event->topic, tlen);
            topic_buf[tlen] = '\0';

            // 2) Copia segura del payload a string C
            char data_buf[96];
            int dlen = event->data_len;
            if (dlen >= (int)sizeof(data_buf)) dlen = sizeof(data_buf) - 1;
            memcpy(data_buf, event->data, dlen);
            data_buf[dlen] = '\0';

            // Logs legibles
            ESP_LOGI(TAG, "Mensaje recibido: topic=%s data=%s", topic_buf, data_buf);

            // Tópicos esperados
            char topic_speedX[64], topic_speedY[64], topic_micro[64], topic_enabled[64], topic_co2[64];
            snprintf(topic_speedX, sizeof(topic_speedX), "esp32/%s/motor/speedX", client_id);
            snprintf(topic_speedY, sizeof(topic_speedY), "esp32/%s/motor/speedY", client_id);
            snprintf(topic_micro,  sizeof(topic_micro),  "esp32/%s/motor/microstepping", client_id);
            snprintf(topic_enabled,sizeof(topic_enabled),"esp32/%s/motor/start", client_id);
            snprintf(topic_co2,    sizeof(topic_co2),    "esp32/%s/sensor/measure", client_id);

            // 3) Compara LONGITUD + CONTENIDO para que sea match exacto
            auto topic_eq = [](const char* a, const char* b){
                return (strlen(a) == strlen(b)) && (strcmp(a,b) == 0);
            };

            // 4) Convierte payload a int con strtol (mejor que atoi)
            char *endp = nullptr;
            long v = strtol(data_buf, &endp, 10);
            bool ok_number = (endp != data_buf); // algo se parseó

            if (topic_eq(topic_buf, topic_speedX) && ok_number) {
                SpeedX = (int)v;
            } else if (topic_eq(topic_buf, topic_speedY) && ok_number) {
                SpeedY = (int)v;
            } else if (topic_eq(topic_buf, topic_micro) && ok_number) {
                micro_stepping = (int)v;
            } else if (topic_eq(topic_buf, topic_enabled) && ok_number) {
                enable_motors = (v != 0);
            } else if (topic_eq(topic_buf, topic_co2) && ok_number) {
                continuousMeasurement = (v != 0);
            }
            break;
        }
        default:
            break;
    }
}
static void heartbeat_task(void *pvParameters) {
    while (1) {
        char topic[64];
        snprintf(topic, sizeof(topic), "esp32/heartbeat/%s", client_id);
        int msg_id = esp_mqtt_client_publish(client, topic, "online", 0, 1, 0);
        ESP_LOGI(TAG, "Heartbeat publicado: topic=%s, msg_id=%d", topic, msg_id);
        vTaskDelay(pdMS_TO_TICKS(5000)); // 5 segundos
    }
}

void mqtt_start(const char *uri) {
    uint8_t mac[6];
    esp_err_t err = esp_wifi_get_mac(WIFI_IF_STA, mac); // <-- CORREGIDO

    snprintf(client_id, sizeof(client_id), "ESP32_%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    esp_mqtt_client_config_t mqtt_cfg = {}; // <-- CORREGIDO
    mqtt_cfg.broker.address.uri = uri;
    mqtt_cfg.credentials.client_id = client_id;

    client = esp_mqtt_client_init(&mqtt_cfg);

    esp_mqtt_client_register_event(client, MQTT_EVENT_ANY, mqtt_event_handler_cb, NULL); // <-- CORREGIDO

    esp_mqtt_client_start(client);

    // Inicia la tarea de heartbeat
    xTaskCreate(heartbeat_task, "heartbeat_task", 4098, NULL, 5, NULL);
}

void enviar_status() {
    char topic[64];
    snprintf(topic, sizeof(topic), "esp32/%s/status", client_id);

    char status[256];
    snprintf(status, sizeof(status),
        "{\"motorX_speed\":%d,"
        "\"motorY_speed\":%d,"
        "\"microstepping\":%d,"
        "\"motors_enabled\":%s,"
        "\"measure\":%s}",
        SpeedX,
        SpeedY,
        micro_stepping,
        enable_motors ? "true" : "false",
        continuousMeasurement ? "true" : "false"
    );

    esp_mqtt_client_publish(client, topic, status, 0, 0, 0);
}