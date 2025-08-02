
#include <stdint.h>
#include "driver/uart.h"

int ModRTU_CRC(uint8_t n1);
void LP8_procesarLectura(uint8_t n);
void LP8_sendRequest(uart_port_t uart_num, const uint8_t *packet, size_t length);

class LP8{
    public:
    LP8();

    void SetPort(uart_port_t);
    void SetTime_Out(uint32_t);
    void SendRequest(const uint8_t *packet);
    int ModRTU_CRC(uint8_t);
    bool ProcesarLectura(uint8_t);
    private:

    uart_port_t _uart_port; //definimos el puerto en el que estará
    
    //definimos el tiempo de espera de respuesta
    uint32_t _time_out_ms; //definimos el tiempo a considerar en MS para la espera de datos
    

    const uint8_t *_packet; //definimos el puntero al arreglo dónde se guardará el paquete
    uint8_t _Count_Send_Request;//contador de cuantas veces enviamos solicitud
    uint8_t _response[49]; //donde se guardan los datos del paquete

    uint8_t _Send_CRC_High; //CRC de comprobación del paquete que se ENVÍA
    uint8_t _Send_CRC_Low;

    uint8_t _Receive_CRC_High; //CRC de comprobación del paquete que se recibe
    uint8_t _Receive_CRC_Low;



};