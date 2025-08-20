#include "LP8_libreria.hpp"

// Puedes dejar los arrays como estáticos fuera de la clase, o ponerlos como miembros estáticos si prefieres
static uint8_t write_partida[]   = {0xfe, 0x41, 0x00, 0x80, 0x01, 0x10, 0x28, 0x7e};
static uint8_t write_normal[]    = {0xfe, 0x41, 0x00, 0x80, 0x01, 0x20, 0x28, 0x6A};
static uint8_t write_calibrar[]  = {0xfe, 0x41, 0x00, 0x80, 0x01, 0x53, 0x69, 0x8F};
static uint8_t read_32_bytes[]   = {0xfe, 0x44, 0x00, 0x80, 0x20, 0x79, 0x3C};
static uint8_t read_44_bytes[]   = {0xfe, 0x44, 0x00, 0x80, 0x2c, 0x79, 0x39};
static uint8_t read_4_bytes[]    = {0xfe, 0x44, 0x00, 0xA4, 0x4, 0x62, 0x27};

void LP8::task_medicion_continua(void *pvParameters) {
    LP8 *sensor = (LP8 *)pvParameters;
    while (1) {
        if (continuousMeasurement) {
            sensor->medir();
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
    bool rdyBajo = false;
    while (gpio_get_level(_rdy1) != 0 && (esp_timer_get_time() - start < 500 * 1000)) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    if (gpio_get_level(_rdy1) == 0) {
        rdyBajo = true;
        printf("rdy1 pasó a LOW correctamente\n");
    } else {
        printf("⏱ Timeout esperando rdy1 en LOW\n");
    }

    // Enviar comando según estado
    if (GetCalibrar()) {
        SendRequest(write_calibrar);
    } else if (GetFirstSense()) {
        SendRequest(write_partida);
    } else {
        SendRequest(write_normal);
    }

    // Esperar a que rdy1 vuelva a HIGH (timeout 1000 ms)
    start = esp_timer_get_time();
    bool rdyAlto = false;
    while (gpio_get_level(_rdy1) != 1 && (esp_timer_get_time() - start < 1000 * 1000)) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    if (gpio_get_level(_rdy1) == 1) {
        rdyAlto = true;
        printf("rdy1 volvió a HIGH correctamente\n");
    } else {
        printf("⏱ Timeout esperando rdy1 en HIGH\n");
    }

    // Enviar read_44_bytes y procesar lectura
    SendRequest(read_44_bytes);
    ProcesarLectura(49); // Asumiendo tamaño 49

    // mostrar_co2(); // Implementa esta función según tu lógica

    printf("=== Fin del ciclo de medición ===\n\n");
}

