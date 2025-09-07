#include "driver/uart.h"
#include "esp_timer.h"
#include <stdint.h>
#include "LP8_libreria.hpp"


bool continuousMeasurement = false;




LP8::LP8(gpio_num_t vbb_en_pin /*= GPIO_NUM_5*/, gpio_num_t rdy_pin /*= GPIO_NUM_18*/, uart_port_t uart_port /*= UART_NUM_2*/) {

    
    _time_out_ms = 10000; //tiempo de rechazo si no hay respuesta
    _Count_Send_Request = 0;//cuantas veces hemos leido el nivel de C02
    _time_sense = 20000;//cada cuanto tiempo hacemos la medicion. No se recomienda mediciones menores a 20 segundos
    
    //puertos
    _uart_port = uart_port;
    _vbb_en1 = vbb_en_pin; //pin de encendido OUTPUT
    _rdy1 = rdy_pin; //pin de lectura INPUT

    //flags
    _calibrar = false; //flag de calibración
    _crc_flag = false; //flag de chequeo de CRC
    _first_sense = false;

    //variables de lectura
    _error[0] = 0;
    _error[1] = 0;
    _error[2] = 0;
    _error[3] = 0;
    _vcap1 = 0;
    _vcap2 = 0;
    _presion = 0;
    _C02 = 0;

    //variables internas
    _size_receive = 0;
    _size_send = 0;


}
void LP8::SetPort(uart_port_t uart_port){
  _uart_port = uart_port;
}

uart_port_t LP8::GetPort() {
  return _uart_port;
}

void LP8::SetTime_Out(uint32_t time_out_ms){
  _time_out_ms = time_out_ms;
}

uint32_t LP8::GetTime_Out(){
  return _time_out_ms;
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

void LP8::SetFirst_Sense(bool first_sense){
  _first_sense = first_sense;
}

void LP8::Setup() {
    gpio_set_direction(_vbb_en1, GPIO_MODE_OUTPUT);
    gpio_set_direction(_rdy1, GPIO_MODE_INPUT);

    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_2,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(_uart_port, &uart_config);

    // Selecciona pines según el puerto UART
    gpio_num_t tx, rx;
    switch (_uart_port) {
        case UART_NUM_1:
            tx = GPIO_NUM_4;   // ejemplo
            rx = GPIO_NUM_5;   // ejemplo
            break;
        case UART_NUM_2:
            tx = GPIO_NUM_17;  // ejemplo
            rx = GPIO_NUM_16;  // ejemplo
            break;
        default:
            tx = GPIO_NUM_1;   // UART0 por defecto
            rx = GPIO_NUM_3;
            break;
    }

    uart_set_pin(_uart_port, tx, rx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(_uart_port, 1024 * 2, 0, 0, NULL, 0);
}

bool LP8::GetCalibrar() {
    return _calibrar;
}

void LP8::SetCalibrar(bool calibrar) {
    _calibrar = calibrar;
}

void LP8::ModRTU_CRC(){
  uint16_t crc = 0xFFFF;
  for (int pos = 0; pos < _size_receive-2; pos++) {
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
  _Receive_CRC_High = (crc >> 8) & 0xFF;
  _Receive_CRC_Low = crc & 0xFF;
}

void LP8::SendRequest(const uint8_t *packet) {

    if (packet[1] == 0x44) {
        _size_receive = packet[4] + 5;
        _size_send = 7;
    } else {
        _size_receive = 4;
        _size_send = 8;
    }

    printf("Esperando disponibilidad del puerto serial...\n");
    uart_write_bytes(_uart_port, (const char *)packet, _size_send);
    uart_wait_tx_done(_uart_port, pdMS_TO_TICKS(100));

   
    bool startFound = false;

    uint8_t buf[4] = {0, 0, 0, 0};
    int64_t start_time = esp_timer_get_time(); // microsegundos
    while (!startFound && ((esp_timer_get_time() - start_time) < (_time_out_ms * 1000))) {
        // Desplaza los valores ANTES de leer el nuevo byte
        buf[0] = buf[1];
        buf[1] = buf[2];
        buf[2] = buf[3];
        if (uart_read_bytes(_uart_port, &buf[3], 1, pdMS_TO_TICKS(10)) > 0) {
            printf("buf[0]=%02X buf[1]=%02X buf[2]=%02X buf[3]=%02X\n", buf[0], buf[1], buf[2], buf[3]);
            // Detecta patrón FE 41 81 E0
            if (buf[0] == 0xFE && buf[1] == 0x41 && buf[2] == 0x81 && buf[3] == 0xE0) {
                for (int i = 0; i < 4; i++) _response[i] = buf[i];
                printf("Inicio de mensaje detectado: 0xFE\n");
                startFound = true;
                break;
            }
            // Detecta patrón FE 44 2C 00
            if (buf[0] == 0xFE && buf[1] == 0x44 && buf[2] == 0x2C && buf[3] == 0x00) {
                for (int i = 0; i < 4; i++) _response[i] = buf[i];
                printf("Inicio de mensaje detectado: 0xFE\n");
                startFound = true;
                break;
            }
        }
    }
    int64_t lectura_fin = esp_timer_get_time();
    vTaskDelay(pdMS_TO_TICKS(10));
        
    printf("Tiempo de lectura de respuesta: %lld ms\n", (lectura_fin - start_time) / 1000);
    int i = 4;
    

    // Imprime los primeros 4 bytes
    for (int j = 0; j < 4; j++) {
        printf("response[%d] = %02X\n", j, _response[j]);
    }

    // Justo después de leer los primeros 4 bytes en _response
    if (_response[1] == 0x41) {
        _size_receive = 4;  // Solo 4 bytes para comando 0x41
    } else if (_response[1] == 0x44) {
        _size_receive = 49; // 49 bytes para comando 0x44
    } else {
        printf("Advertencia: comando desconocido 0x%02X\n", _response[1]);
        _size_receive = 4; // Valor seguro por defecto
    }
    printf("size_receive ajustado: %d\n", _size_receive);

  start_time = esp_timer_get_time();

    // Leer el resto de la respuesta
    while (i < _size_receive && ((esp_timer_get_time() - start_time) < (_time_out_ms * 1000))) {
        if (uart_read_bytes(_uart_port, &_response[i], 1, pdMS_TO_TICKS(10)) > 0) {
            printf("response[%d] = %02X\n", i, _response[i]);
            i++;
        }
    }
    lectura_fin = esp_timer_get_time();
    printf("Tiempo de lectura de respuesta: %lld ms\n", (lectura_fin - start_time) / 1000);
    if (i < _size_receive) {
        printf("Timeout leyendo respuesta completa.\n");
        return;
    }

    printf("Mensaje completo recibido.\n");
}

void LP8::ProcesarLectura(){
    // Calcular CRC
    ModRTU_CRC();


    _CRC_Low = _response[_size_receive-2];
    _CRC_High = _response[_size_receive-1];

    if ((_CRC_High != _Receive_CRC_High) || (_CRC_Low != _Receive_CRC_Low)) {
        printf("! ! ! El CRC calculado no coincide con el CRC del sensor.\n");
        printf("crc_High = %02X\n", _response[_size_receive-2]);
        printf("crc_Low = %02X\n", _response[_size_receive-1]);
        SetFlag_CRC(false);
        for(int i = 0; i < _size_receive; i++){
            printf("%02X\n", _response[i]);
        }
        return;
    } else {
        printf("El CRC calculado coincide con el CRC del sensor.\n");


        // Mostrar presión si hay datos suficientes
        if(_size_receive > 28){
            int16_t rawValue = (int16_t)((_response[27] << 8) | _response[28]);
            _presion = rawValue * 0.1f; // Escala a hPa
            printf("Mostrando Host Pressure\n");
            printf("Presión: %.1f hPa\n", _presion);
        }

        if(_size_receive > 30){
            int co2 = (_response[29] << 8) | _response[30];
            _C02 = (float)co2;
            printf("CO2: %d ppm\n", co2);
        }
        // Mostrar Vcap1 y Vcap2 si hay datos suficientes
        if(_size_receive > 39){
            _vcap1 = (uint16_t)((_response[35] << 8) | _response[36]);
            _vcap2 = (uint16_t)((_response[37] << 8) | _response[38]);
            printf("Vcap1 %u mV\n", _vcap1);
            printf("Vcap2 %u mV\n", _vcap2);
            
        }

        // Mostrar bits de error si hay datos suficientes
        if(_size_receive > 43){
            printf("Mostrando bits de errores:\n");
            printf("Error bit 3: %u\n", _response[39]);
            printf("Error bit 2: %u\n", _response[40]);
            printf("Error bit 1: %u\n", _response[41]);
            printf("Error bit 0: %u\n", _response[42]);
            _error[3] = _response[39];
            _error[2] = _response[40];
            _error[1] = _response[41];
            _error[0] = _response[42];
        }

        SetFlag_CRC(true);

    }
}

void LP8::Reset_receive(){
  for(int i = 0; i < 49; i++){
    _response[i] = 0;
  }
}

void LP8::SetFlag_CRC(bool flag){
  _crc_flag = flag;
}

float LP8::Get_C02(){
  return _C02;
}

bool LP8::GetCrc_flag(){
  return _crc_flag;
}

float LP8::GetPresion(){
  return _presion;
}

uint16_t LP8::GetVcap1(){
  return _vcap1;
}

uint16_t LP8::GetVcap2(){
  return _vcap2;
}


uint8_t LP8::GetError(uint8_t index){
  if(index > 3) {
    return 0xFF; // o el valor que prefieras para error
  }
  return _error[index];
}
