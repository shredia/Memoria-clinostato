#pragma once
#include "LP8_libreria.hpp"

// Función global de la tarea
void LP8_task_medicion_continua(void *pvParameters);


// Arrays de comandos
extern uint8_t write_partida[8];
extern uint8_t write_normal[8];
extern uint8_t write_calibrar[8];
extern uint8_t read_32_bytes[7];
extern uint8_t read_44_bytes[7];
extern uint8_t read_4_bytes[7];

// Variable externa para medición continua
extern bool continuousMeasurement;