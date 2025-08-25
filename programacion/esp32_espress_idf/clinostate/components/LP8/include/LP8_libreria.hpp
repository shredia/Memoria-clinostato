#ifndef LP8_LIBRERIA_HPP
#define LP8_LIBRERIA_HPP

#include <stdint.h>
#include "driver/uart.h"
#include "driver/gpio.h"

class LP8 {
public:
    LP8(gpio_num_t vbb_en_pin = GPIO_NUM_5, gpio_num_t rdy_pin = GPIO_NUM_18, uart_port_t uart_port = UART_NUM_2);

    void Setup();
    void SetPort(uart_port_t uart_port);
    void SetTime_Out(uint32_t time_out_ms);
    void SetTime_Sense(uint32_t time_sense);

    uint32_t GetTime_Sense();
    uint32_t GetTime_Out();
    bool GetFirst_Sense();
    void SetFirst_Sense(bool first_sense);
    bool GetCalibrar();
    void SetCalibrar(bool calibrar);
    uart_port_t GetPort();

    void SendRequest(const uint8_t *packet);
    void ModRTU_CRC();
    void ProcesarLectura();

    void Reset_receive();
    void SetFlag_CRC(bool flag);

    float Get_C02();
    bool GetCrc_flag();
    float GetPresion();
    uint16_t GetVcap1();
    uint16_t GetVcap2();
    uint8_t GetError(uint8_t index);
    void task_medicion_continua(void *pvParameters); // <-- Agrega esta línea
    void medir();
    void publicar_datos_sensor();


private:

    uint32_t _time_out_ms;
    uint32_t _time_sense;
    //flags
    bool _first_sense;
    bool _calibrar;
    bool _crc_flag;

    // Pines
    gpio_num_t _vbb_en1;
    gpio_num_t _rdy1;
    uint8_t _Count_Send_Request;
    uart_port_t _uart_port;

    //variables de lectura
    uint8_t _error[4];
    uint16_t _vcap1;
    uint16_t _vcap2;
    float _presion;
    float _C02;

    //variables internas
    uint8_t _size_receive;
    uint8_t _size_send;
    uint8_t _response[49];
    uint8_t _CRC_High;
    uint8_t _CRC_Low;
    uint8_t _Receive_CRC_High;
    uint8_t _Receive_CRC_Low;
};

#endif // LP8_LIBRERIA_HPP