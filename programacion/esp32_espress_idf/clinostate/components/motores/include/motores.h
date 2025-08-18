#pragma once

#include "AccelStepper.h"
#include <driver/gpio.h>

extern AccelStepper MotorX;
extern AccelStepper MotorY;



void motores_setup(void);
void setMicrostepping(int ms);
void actualizarMotores();

extern gpio_num_t ENABLE_PIN;
extern int micro_stepping;
extern bool enable_motors;
extern int SpeedX;
extern int SpeedY;
