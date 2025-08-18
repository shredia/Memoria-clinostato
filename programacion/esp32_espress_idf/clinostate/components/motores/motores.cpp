#include "AccelStepper.h"
#include <driver/gpio.h>
#include "motores.h"
#include "esp_log.h"
// Pines
#define DIR_PINX ((gpio_num_t)13)
#define STEP_PINX ((gpio_num_t)12)
#define DIR_PINY ((gpio_num_t)14)
#define STEP_PINY ((gpio_num_t)27)
#define MS1_PIN   ((gpio_num_t)25)
#define MS2_PIN   ((gpio_num_t)33)
#define MS3_PIN   ((gpio_num_t)32)

gpio_num_t ENABLE_PIN = (gpio_num_t)26;
int SpeedX = 0;
int SpeedY = 0;
int micro_stepping = 0;
bool enable_motors = false;

AccelStepper MotorX(AccelStepper::DRIVER,STEP_PINX, DIR_PINX );
AccelStepper MotorY(AccelStepper::DRIVER, STEP_PINY, DIR_PINY);

void motores_setup(void) {
        MotorX.setMaxSpeed(4000);
        MotorY.setMaxSpeed(4000);

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

void actualizarMotores() {
    if (enable_motors)
    {   
        gpio_set_level(ENABLE_PIN, 0);
        setMicrostepping(micro_stepping);
        MotorX.setSpeed(SpeedX);
        MotorY.setSpeed(SpeedY);
        
        MotorX.runSpeed();
        MotorY.runSpeed();
    } else 
    {   
        ESP_LOGI("Actualizar_motores", "Disabled");
        gpio_set_level(ENABLE_PIN, 1); // Deshabilita el driver
        MotorX.stop();
        MotorY.stop();
    }
}

