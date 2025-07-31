#include "driver/uart.h"



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
uint8_t response[49];

int ModRTU_CRC(uint8_t n1){
  uint16_t crc = 0xFFFF;
  for (int pos = 0; pos < n1; pos++) {
    crc ^= (uint16_t)response[pos];
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


void LP8_sendRequest(HardwareSerial &SerialPort, byte packet[]) {
  int n,m;
  if(packet[1] == 0x44){ //si es 0x44, es solicitud de read, si es 0x41 es solicitud de write
    n = packet[4] + 5; //en la ubicación packet[2] están la cantidad de datos solicitados a leer
    m = 7;
  }else
  {
  n = 4;
  m = 8;
  }
    printf("Esperando disponibilidad del puerto serial... \n");
    SerialPort.write(packet, m);
  // Esperar el byte de inicio FE
  unsigned long startTime = millis();
  bool startFound = false;
  
  
  while (millis() - startTime < 5000) {
    if (SerialPort.available()) {
      byte b = SerialPort.read();
      printf("Byte de inicio leido:");
      printf("%02X\n",b);
      if (b == 0xFE) {
        response[0] = b;
        printf("Inicio de mensaje detectado: 0xFE \n");
        startFound = true;
        break;
      }
    }
    delay(10);
  }

  if (!startFound) {
    printf("Timeout esperando byte de inicio FE. \n");
    return;
  }

  // Leer los siguientes n - 1 bytes con timeout
  int i = 1;
  startTime = millis();  // Reiniciar tiempo
      printf("response[");
      printf("0");
      printf("] = ");
      printf("%02X\n",response[0]);
  while (i < n && millis() - startTime < 1000) {
    if (SerialPort.available()) {
      response[i] = SerialPort.read();
      printf("response[");
      printf(i);
      printf("] = ");
      printf("%02X\n",response[i]);
      i++;
    }
  }

  if (i < n) {
    printf("Timeout leyendo respuesta completa.\n");
    return;
  }

  printf("Mensaje completo recibido. \n");
  
}

void LP8_procesarLectura(int n) {
  
  // Calcular CRC
  uint16_t crc_result = ModRTU_CRC(n-2);
  printf("CRC calculado: ");
  printf("%02X\n",crc_result);

  int crc_result_h = crc_result & 0xff;
  int crc_result_l = (crc_result >> 8) & 0xff;

  if ((response[n-2] != crc_result_h) || (response[n-1] != crc_result_l)) {
    printf("! ! ! El CRC calculado no coincide con el CRC del sensor.\n");
    printf("crc_High = ");
    printf("%02X\n",response[n-2]);
    printf("crc_Low = ");
    printf("%02X\n",response[n-1]);
    
    for(int i = 0; i < n; i++){
      printf("%02X\n",response[i]);
    }

  } else {
    printf("El CRC calculado coincide con el CRC del sensor. \n");
  }
}

