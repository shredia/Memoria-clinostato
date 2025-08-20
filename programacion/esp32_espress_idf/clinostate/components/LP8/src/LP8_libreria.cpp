#include "driver/uart.h"
#include "esp_timer.h"
#include <stdint.h>
#include "LP8_libreria.hpp"


bool continuousMeasurement = false;

// Pines UART del sensor LP8
#define RX_uart1 GPIO_NUM_16
#define TX_uart1 GPIO_NUM_17
#define vbb_en1 GPIO_NUM_5
#define rdy1  GPIO_NUM_18



LP8::LP8(gpio_num_t vbb_en_pin /*= GPIO_NUM_5*/, gpio_num_t rdy_pin /*= GPIO_NUM_18*/, uart_port_t uart_port /*= UART_NUM_2*/) {
    _first_sense = false;
    _uart_port = uart_port; // ahora configurable y predefinido
    _time_out_ms = 1000;
    _Count_Send_Request = 0;
    _time_sense = 20000;
    _vbb_en1 = vbb_en_pin;
    _rdy1 = rdy_pin;
    _calibrar = false;

}
void LP8::SetPort(uart_port_t uart_port){
  _uart_port = uart_port;
}

void LP8::SetTime_Out(uint32_t time_out_ms){
  _time_out_ms = time_out_ms;
}

void LP8::SetTime_Sense(uint32_t time_sense){
  _time_sense = time_sense;
}

uint32_t LP8::GetTime_Sense(){
  return _time_sense;
}

bool LP8::GetFirst_Sense(){
  return _first_sense;
}

void LP8::Setup(){
  gpio_set_direction(_vbb_en1, GPIO_MODE_OUTPUT);
  gpio_set_direction(_rdy1, GPIO_MODE_INPUT);
}

bool LP8::GetCalibrar() {
    return _calibrar;
}

int LP8::ModRTU_CRC(uint8_t n1){
  uint16_t crc = 0xFFFF;
  for (int pos = 0; pos < n1; pos++) {
    crc ^= (uint16_t)_response[pos];
    for (int i = 0; i < 8; i++) {
      if (crc & 0x0001) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }

  printf("CRC calculado: ");
  printf("%02X\n",crc);
  return crc;
}

void LP8::SendRequest(const uint8_t *packet) {
    int size_receive, size_send;
    if (packet[1] == 0x44) {
        size_receive = packet[4] + 5;
        size_send = 7;
    } else {
        size_receive = 4;
        size_send = 8;
    }

    printf("Esperando disponibilidad del puerto serial...\n");
    uart_write_bytes(_uart_port, (const char *)packet, size_send);
    uart_wait_tx_done(_uart_port, pdMS_TO_TICKS(100));

    // Esperar el patrón de inicio
    int TIMEOUT_MS = 5000;
    int64_t start_time = esp_timer_get_time(); // microsegundos
    bool startFound = false;
    uint8_t b0 = 0, b1 = 0, b2 = 0, b3 = 0;

    while (!startFound && ((esp_timer_get_time() - start_time) < TIMEOUT_MS * 1000)) {
        if (uart_read_bytes(_uart_port, &b3, 1, pdMS_TO_TICKS(10)) > 0) {
            b0 = b1;
            b1 = b2;
            b2 = b3;

            printf("%02X\n", b3);

            bool pat1 = (b0 == 0xFE && b1 == 0x41 && b2 == 0x81 && b3 == 0xE0);
            bool pat2 = (b0 == 0xFE && b1 == 0x44 && b2 == 0x2C && b3 == 0x00);

            if (pat1 || pat2) {
                _response[0] = b0; _response[1] = b1; _response[2] = b2; _response[3] = b3;
                printf("Inicio de mensaje detectado: 0xFE\n");
                printf("Comando encontrado: 0x%02X\n", _response[1]);
                printf("bit 3: 0x%02X\n", _response[2]);
                printf("bit 4: 0x%02X\n", _response[3]);
                startFound = true;
                break;
            }
        }
    }
    vTaskDelay(pdMS_TO_TICKS(10));

    int i = 4;
    int64_t read_start_time = esp_timer_get_time();

    // Imprime los primeros 4 bytes
    for (int j = 0; j < 4; j++) {
        printf("response[%d] = %02X\n", j, _response[j]);
    }

    if (size_receive != _response[1]) {
        printf("Error, se saltó una lectura de respuesta. Probablemente la respuesta de algún comando write\n");
        printf("size_receive : %d\n", size_receive);
        size_receive = (_response[1] == 0x44) ? packet[4] + 5 : 4;
    }

    // Leer el resto de la respuesta
    while (i < size_receive && ((esp_timer_get_time() - read_start_time) < 1000 * 1000)) {
        if (uart_read_bytes(_uart_port, &_response[i], 1, pdMS_TO_TICKS(10)) > 0) {
            printf("response[%d] = %02X\n", i, _response[i]);
            i++;
        }
    }

    if (i < size_receive) {
        printf("Timeout leyendo respuesta completa.\n");
        return;
    }

    printf("Mensaje completo recibido.\n");
}

bool LP8::ProcesarLectura(uint8_t size_receive){
    // Calcular CRC
    uint16_t crc_result = ModRTU_CRC(size_receive-2);
    printf("CRC calculado: %04X\n", crc_result);

    int crc_result_h = crc_result & 0xff;
    int crc_result_l = (crc_result >> 8) & 0xff;

    if ((_response[size_receive-2] != crc_result_h) || (_response[size_receive-1] != crc_result_l)) {
        printf("! ! ! El CRC calculado no coincide con el CRC del sensor.\n");
        printf("crc_High = %02X\n", _response[size_receive-2]);
        printf("crc_Low = %02X\n", _response[size_receive-1]);
        for(int i = 0; i < size_receive; i++){
            printf("%02X\n", _response[i]);
        }
        return false;
    } else {
        printf("El CRC calculado coincide con el CRC del sensor.\n");

        // Mostrar presión si hay datos suficientes
        if(size_receive > 28){
            int16_t rawValue = (int16_t)((_response[27] << 8) | _response[28]);
            float presion = rawValue * 0.1f; // Escala a hPa
            printf("Mostrando Host Pressure\n");
            printf("Presión: %.1f hPa\n", presion);
        }

        // Mostrar Vcap1 y Vcap2 si hay datos suficientes
        if(size_receive > 39){
            uint16_t vcap1 = (uint16_t)((_response[35] << 8) | _response[36]);
            uint16_t vcap2 = (uint16_t)((_response[37] << 8) | _response[38]);
            printf("Vcap1 %u mV\n", vcap1);
            printf("Vcap2 %u mV\n", vcap2);
        }

        // Mostrar bits de error si hay datos suficientes
        if(size_receive > 43){
            printf("Mostrando bits de errores:\n");
            printf("Error bit 3: %u\n", _response[39]);
            printf("Error bit 2: %u\n", _response[40]);
            printf("Error bit 1: %u\n", _response[41]);
            printf("Error bit 0: %u\n", _response[42]);
        }

        return true;
    }
}

