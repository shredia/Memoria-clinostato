#pragma once

#ifndef ACCELSTEPPER_WRAPPER_H
#define ACCELSTEPPER_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif


#include <driver/gpio.h>


void motores_setup(void); //definimos la configuración de los drivers de los motores

void setMicrostepping(int ms); //configuramos el micro_stepping de ambos motores.(ambos se configuran al mismo micro_stepping)


#ifdef __cplusplus
}
#endif

#endif