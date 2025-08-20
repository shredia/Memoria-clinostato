#include <stdint.h>
#include "driver/uart.h"
#include "driver/gpio.h"

int ModRTU_CRC(uint8_t n1);
void LP8_procesarLectura(uint8_t n);
void LP8_sendRequest(uart_port_t uart_num, const uint8_t *packet, size_t length);

extern bool continuousMeasurement;
class LP8{
public:
    LP8(gpio_num_t vbb_en_pin = GPIO_NUM_5, gpio_num_t rdy_pin = GPIO_NUM_18, uart_port_t uart_port = UART_NUM_2);
    void Setup();
    void SetPort(uart_port_t);
    void SetTime_Out(uint32_t);
    void SetTime_Sense(uint32_t);
    uint32_t GetTime_Sense();
    bool GetFirst_Sense();
    bool GetCalibrar();
    void SendRequest(const uint8_t *packet);
    int ModRTU_CRC(uint8_t);
    bool ProcesarLectura(uint8_t);
    void task_medicion_continua(void *pvParameters);
    void medir();
private:
    uart_port_t _uart_port;
    uint32_t _time_out_ms;
    uint32_t _time_sense;
    bool _first_sense;
    bool _calibrar;
    gpio_num_t _vbb_en1;
    gpio_num_t _rdy1;
    uint8_t _Count_Send_Request;
    uint8_t _response[49];
    uint8_t _Send_CRC_High;
    uint8_t _Send_CRC_Low;
    uint8_t _Receive_CRC_High;
    uint8_t _Receive_CRC_Low;
};