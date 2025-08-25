#pragma once
#include "i2c_bus.h"
#include "bme280.h"
extern bool continuousMeasurement;
void bme280_setup();
void bme280_leer(float &temperature, float &humidity, float &pressure);
void bme280_start_periodic();
void bme280_task(void *pvParameters);
