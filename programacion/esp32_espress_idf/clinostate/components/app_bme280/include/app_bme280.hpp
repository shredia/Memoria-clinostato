#ifndef APP_BME280_HPP
#define APP_BME280_HPP

#ifdef __cplusplus
extern "C" {
#endif

extern bool Flag_enable_BME280;
void bme280_setup();
void bme280_leer(float &temperature, float &humidity, float &pressure);
void bme280_task(void *pvParameters);
void bme280_start_periodic();

#ifdef __cplusplus
}
#endif

#endif // APP_BME280_HPP