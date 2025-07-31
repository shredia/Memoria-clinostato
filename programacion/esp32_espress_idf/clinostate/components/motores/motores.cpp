#include "AccelStepper.h"
#include <driver/gpio.h>

extern "C" {

// Pines redefinidos como gpio_num_t
#define DIR_PINX   GPIO_NUM_27
#define STEP_PINX  GPIO_NUM_14  
#define DIR_PINY   GPIO_NUM_25
#define STEP_PINY  GPIO_NUM_33
#define ENABLE_PIN GPIO_NUM_13
#define MS1_PIN    GPIO_NUM_12
#define MS2_PIN    GPIO_NUM_32 
#define MS3_PIN    GPIO_NUM_26

// Motores declarados fuera del bloque extern "C" para usar clases C++
}

  static AccelStepper stepperX(AccelStepper::DRIVER, DIR_PINX, STEP_PINX); 
  static AccelStepper stepperY(AccelStepper::DRIVER, DIR_PINY, STEP_PINY); 

extern "C" void motores_setup(void) {
    gpio_config_t io_conf_ENABLE_PIN = {
        .pin_bit_mask = (1ULL << ENABLE_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf_ENABLE_PIN);

    gpio_config_t io_conf_MS1_PIN = {
        .pin_bit_mask = (1ULL << MS1_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf_MS1_PIN);

    gpio_config_t io_conf_MS2_PIN = {
        .pin_bit_mask = (1ULL << MS2_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf_MS2_PIN);

    gpio_config_t io_conf_MS3_PIN = {
        .pin_bit_mask = (1ULL << MS3_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf_MS3_PIN);
}

extern "C" void setMicrostepping(int ms) {
    gpio_set_level(MS1_PIN, ms & 1);
    gpio_set_level(MS2_PIN, (ms >> 1) & 1);
    gpio_set_level(MS3_PIN, (ms >> 2) & 1);
}
