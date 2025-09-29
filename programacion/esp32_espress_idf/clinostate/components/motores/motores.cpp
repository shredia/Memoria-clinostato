#include "AccelStepper.h"
#include <driver/gpio.h>
#include "motores.h"
#include "esp_log.h"
// Pines
#define DIR_PINX 2
#define STEP_PINX 15
#define DIR_PINY 27
#define STEP_PINY 14
#define MS1_PIN  25
#define MS2_PIN  33
#define MS3_PIN  32
#define ENABLE_PIN 26

int SpeedX = 0;
int SpeedY = 0;
int micro_stepping = 0;
bool enable_motors = false;

// variables estáticas para detectar cambios
static bool prev_enable = false;
static int  prev_micro  = -1;
static float prev_vx = 0, prev_vy = 0;

AccelStepper MotorX(AccelStepper::DRIVER, (gpio_num_t)STEP_PINX, (gpio_num_t)DIR_PINX );
AccelStepper MotorY(AccelStepper::DRIVER, (gpio_num_t)STEP_PINY, (gpio_num_t)DIR_PINY );

void motores_setup(void) {
    MotorX.setMaxSpeed(4000);
    MotorY.setMaxSpeed(4000);
    MotorX.setAcceleration(100);
    MotorY.setAcceleration(100);
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

    gpio_config_t io_conf_STEPX = {
        .pin_bit_mask = (1ULL << STEP_PINX),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf_STEPX);

    gpio_config_t io_conf_DIRX = {
        .pin_bit_mask = (1ULL << DIR_PINX),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf_DIRX);

    // Pines STEP y DIR MotorY
    gpio_config_t io_conf_STEPY = {
        .pin_bit_mask = (1ULL << STEP_PINY),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf_STEPY);

    gpio_config_t io_conf_DIRY = {
        .pin_bit_mask = (1ULL << DIR_PINY),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf_DIRY);
}

void setMicrostepping(int ms) {
    gpio_set_level((gpio_num_t)MS1_PIN, ms & 1);
    gpio_set_level((gpio_num_t)MS2_PIN, (ms >> 1) & 1);
    gpio_set_level((gpio_num_t)MS3_PIN, (ms >> 2) & 1);
}

void actualizarMotores() {
    // Habilitar/deshabilitar SOLO si cambia
    if (enable_motors != prev_enable) {
        gpio_set_level((gpio_num_t)ENABLE_PIN, enable_motors ? 0 : 1);
        prev_enable = enable_motors;
    }

    if (!enable_motors) return;

    // Microstepping SOLO si cambia
    if (micro_stepping != prev_micro) {
        setMicrostepping(micro_stepping);
        prev_micro = micro_stepping;
    }

    // Actualiza velocidades SOLO si cambian
    if (SpeedX != prev_vx) { MotorX.setSpeed(SpeedX); prev_vx = SpeedX; }
    if (SpeedY != prev_vy) { MotorY.setSpeed(SpeedY); prev_vy = SpeedY; }

    // Avance no bloqueante
    MotorX.runSpeed();
    MotorY.runSpeed();
}
