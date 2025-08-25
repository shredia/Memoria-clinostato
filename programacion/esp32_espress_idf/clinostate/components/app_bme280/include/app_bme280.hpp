#pragma once
#include "i2c_bus.h"
#include "bme280.h"

void bme280_setup();
void bme280_leer(float &temperature, float &humidity, float &pressure);
void bme280_start_periodic();

extern bool Flag_enable_BME280;