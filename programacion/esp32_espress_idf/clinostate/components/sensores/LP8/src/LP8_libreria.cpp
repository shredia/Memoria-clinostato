#include "driver/uart.h"
#include "esp_timer.h"
#include <stdint.h>
#include "LP8_libreria.hpp"

// Pines UART del sensor LP8
#define RX_uart1 GPIO_NUM_16
#define TX_uart1 GPIO_NUM_17
#define vbb_en1 GPIO_NUM_5
#define rdy1  GPIO_NUM_18

static uint8_t write_partida[] = {0xfe, 0x41, 0x00, 0x80, 0x01, 0x10, 0x28, 0x7e};
static uint8_t write_normal[] = {0xfe, 0x41, 0x00, 0x80, 0x01, 0x20, 0x28, 0x6A};
static uint8_t write_calibrar[] = {0xfe, 0x41, 0x00, 0x80, 0x01, 0x53, 0x69, 0x8F};
static uint8_t read_32_bytes[] = {0xfe, 0x44, 0x00, 0x80, 0x20, 0x79, 0x3C};
static uint8_t read_44_bytes[] = {0xfe, 0x44, 0x00, 0x80, 0x2c, 0x79, 0x39};
static uint8_t read_4_bytes[] = {0xfe, 0x44, 0x00, 0xA4, 0x4, 0x62, 0x27};

LP8::LP8(){
  _uart_port = UART_NUM_2;
  _time_out_ms = 1000;
  _Count_Send_Request = 0;
}
void LP8::SetPort(uart_port_t uart_port){
  _uart_port = uart_port;
}

void LP8::SetTime_Out(uint32_t time_out_ms){
  _time_out_ms = time_out_ms;
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

void LP8::SendRequest(const uint8_t *packet){
  int size_response,size_packet;
  if(packet[1] == 0x44){ //si es 0x44, es solicitud de read, si es 0x41 es solicitud de write
    size_response = packet[4] + 5; //en la ubicación packet[2] están la cantidad de datos solicitados a leer
    size_packet = 7;
  }else
  {
  size_response = 4;
  size_packet = 8;
  }
  _Send_CRC_High = size_packet-2;
  _Send_CRC_Low = size_packet-1;
    printf("Esperando disponibilidad del puerto serial... \n");
    uart_write_bytes(_uart_port, (const char *)packet, size_packet);
    uart_wait_tx_done(_uart_port, pdMS_TO_TICKS(100));

   // Esperar byte de inicio 0xFE
    int len = 0;
    uint8_t b;
    int64_t start_time = esp_timer_get_time(); // microsegundos
  
    while((esp_timer_get_time() - start_time) < _time_out_ms * 1000) {
        len = uart_read_bytes(_uart_port, &b, 1, pdMS_TO_TICKS(10));
        if (len > 0) {
            printf("Byte de inicio leído: %02X\n", b);
            if (b == 0xFE) {
                _response[0] = b;
                printf("Inicio de mensaje detectado: 0xFE\n");
                break;
            }
        }
    }

    if (b != 0xFE) {
        printf("Timeout esperando byte de inicio FE.\n");
        return;
    }

    // Leer el resto de la respuesta
    uint8_t bytes_read = 1;
    start_time = esp_timer_get_time();

    while (bytes_read < size_response && ((esp_timer_get_time() - start_time) < 1000 * 1000)){
        len = uart_read_bytes(_uart_port, &_response[bytes_read], 1, pdMS_TO_TICKS(10));
        if (len > 0) {
            printf("response[%d] = %02X\n", (int)bytes_read, _response[bytes_read]);
            bytes_read++;
        }
    }

    if (bytes_read < size_response) {
        printf("Timeout leyendo respuesta completa.\n");
        return;
    }

    printf("Mensaje completo recibido.\n");
}

bool LP8::ProcesarLectura(uint8_t size_response){
  
  // Calcular CRC
  uint16_t crc_result = ModRTU_CRC(size_response-2);
  printf("CRC calculado: ");
  printf("%02X\n",crc_result);

  int crc_result_h = crc_result & 0xff;
  int crc_result_l = (crc_result >> 8) & 0xff;

  if ((_response[size_response-2] != crc_result_h) || (_response[size_response-1] != crc_result_l)) {
    printf("! ! ! El CRC calculado no coincide con el CRC del sensor.\n");
    printf("crc_High = ");
    printf("%02X\n",_response[size_response-2]);
    printf("crc_Low = ");
    printf("%02X\n",_response[size_response-1]);
    
    
    for(int i = 0; i < size_response; i++){
      printf("%02X\n",_response[i]);
    }
    return 0;
  } else {
    return true;
    printf("El CRC calculado coincide con el CRC del sensor. \n");
  }
}

