#include "AccelStepper.h"
#include <driver/gpio.h>

extern "C" {
    #define DIR_PINX 27
    #define STEP_PINX 14  

    #define DIR_PINY 25
    #define STEP_PINY 33

    #define ENABLE_PIN 13

    #define MS1_PIN 12
    #define MS2_PIN 32 
    #define MS3_PIN 26 
    static AccelStepper stepperX(AccelStepper::DRIVER, DIR_PINX, STEP_PINX); // 
    static AccelStepper steppery(AccelStepper::DRIVER, DIR_PINY, STEP_PINY); // 
void motores_setup(void){
    // Pines de motores y control
    //Serial.printf("Iniciando configuración de pines motores");


    // Estado del sistema
    int speedX = 0;
    int speedY = 0;
    bool dirX = true;
    bool dirY = true;
    bool motorsEnabled = false;
    int microstepping = 1;

    gpio_config_t io_conf_ENABLE_PIN = {
    .pin_bit_mask = (1ULL << ENABLE_PIN),
    .mode = GPIO_MODE_OUTPUT,  // o GPIO_MODE_INPUT
    .pull_up_en = 0,
    .pull_down_en = 0,
    .intr_type = GPIO_INTR_DISABLE
};
    gpio_config(&io_conf_ENABLE_PIN);

    gpio_config_t io_conf_MS1_PIN = {
    .pin_bit_mask = (1ULL << MS1_PIN),
    .mode = GPIO_MODE_OUTPUT,  // o GPIO_MODE_INPUT
    .pull_up_en = 0,
    .pull_down_en = 0,
    .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf_MS1_PIN);

    gpio_config_t io_conf_MS2_PIN = {
    .pin_bit_mask = (1ULL << MS2_PIN),
    .mode = GPIO_MODE_OUTPUT,  // o GPIO_MODE_INPUT
    .pull_up_en = 0,
    .pull_down_en = 0,
    .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf_MS2_PIN);

    gpio_config_t io_conf_MS3_PIN = {
    .pin_bit_mask = (1ULL << MS3_PIN),
    .mode = GPIO_MODE_OUTPUT,  // o GPIO_MODE_INPUT
    .pull_up_en = 0,
    .pull_down_en = 0,
    .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf_MS3_PIN);

}

void setMicrostepping(int ms) {
    if (ms == 0) {
    gpio_set_level(MS1_PIN, 0);
    gpio_set_level(MS2_PIN, 0);
    gpio_set_level(MS3_PIN, 0);
  } else {
    gpio_set_level(MS1_PIN, ms & 1);
    gpio_set_level(MS2_PIN, (ms >> 1) & 1);
    gpio_set_level(MS3_PIN, (ms >> 2) & 1);
  }
}

}