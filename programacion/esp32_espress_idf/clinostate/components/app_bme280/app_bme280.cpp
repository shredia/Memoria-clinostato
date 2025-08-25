#include "app_bme280.hpp"
#include "i2c_bus.h"
#include "bme280.h"
#include "esp_timer.h"
#include "mqtt_client.h"
#include "app_mqtt.h"    

#define I2C_MASTER_NUM      I2C_NUM_0
#define I2C_MASTER_SDA_IO   21
#define I2C_MASTER_SCL_IO   22
#define I2C_MASTER_FREQ_HZ 40000 // Prueba con 40kHz

bool Flag_enable_BME280 = false;
static i2c_bus_handle_t i2c_bus = NULL;
static bme280_handle_t bme280 = NULL;

void bme280_setup() {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master = {
            .clk_speed = I2C_MASTER_FREQ_HZ
        },
        .clk_flags = 0
    };
    i2c_bus = i2c_bus_create(I2C_MASTER_NUM, &conf);
    bme280 = bme280_create(i2c_bus, 0x76);
    bme280_default_init(bme280);
}

void bme280_leer(float &temperature, float &humidity, float &pressure) {
    bme280_read_temperature(bme280, &temperature);
    bme280_read_humidity(bme280, &humidity);
    bme280_read_pressure(bme280, &pressure);
}

// Tarea periódica para leer el BME280
void bme280_task(void *pvParameters) {
    float temperature, humidity, pressure;
    for (;;) {
        if (continuousMeasurement) {
            bme280_leer(temperature, humidity, pressure);
            printf("BME280: Temp=%.2f°C Hum=%.2f%% Pres=%.2f hPa\n", temperature, humidity, pressure );

            // Publicar temperatura
            char topic_temp[64];
            snprintf(topic_temp, sizeof(topic_temp), "esp32/%s/bme280/temp", client_id);
            char msg_temp[32];
            snprintf(msg_temp, sizeof(msg_temp), "%.2f", temperature);
            esp_mqtt_client_publish(client, topic_temp, msg_temp, 0, 1, 0);

            // Publicar humedad
            char topic_hum[64];
            snprintf(topic_hum, sizeof(topic_hum), "esp32/%s/bme280/hum", client_id);
            char msg_hum[32];
            snprintf(msg_hum, sizeof(msg_hum), "%.2f", humidity);
            esp_mqtt_client_publish(client, topic_hum, msg_hum, 0, 1, 0);

            // Publicar presión
            char topic_pres[64];
            snprintf(topic_pres, sizeof(topic_pres), "esp32/%s/bme280/presion", client_id);
            char msg_pres[32];
            snprintf(msg_pres, sizeof(msg_pres), "%.2f", pressure );
            esp_mqtt_client_publish(client, topic_pres, msg_pres, 0, 1, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(5000)); // Espera 5 segundos
    }
}

// Llama esto en tu app_main o setup principal
void bme280_start_periodic() {
    xTaskCreate(bme280_task, "bme280_task", 4096, NULL, 5, NULL);

}