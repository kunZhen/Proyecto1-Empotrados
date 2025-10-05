#pragma once
#ifdef __cplusplus
extern "C" {
#endif

typedef enum { INPUT = 0, OUTPUT = 1 } pinmode_t;

/* GPIO básico */
int gpioExport(int gpio);
int gpioUnexport(int gpio);
int gpioSetActiveLow(int gpio, int active_low);
int pinMode(int gpio, pinmode_t mode);
int digitalWrite(int gpio, int value);
int digitalRead(int gpio);
int blink(int gpio, double freq_hz, double duration_sec);

/* PWM por hardware */
int pwmExport(int pwm_chip, int pwm_channel);
int pwmUnexport(int pwm_chip, int pwm_channel);
int pwmSetPeriod(int pwm_chip, int pwm_channel, unsigned long period_ns);
int pwmSetDutyCycle(int pwm_chip, int pwm_channel, unsigned long duty_ns);
int pwmEnable(int pwm_chip, int pwm_channel, int enable);

/* Motores - PRIMERO las enums y structs */
typedef enum {
    MOTOR_STOP = 0,
    MOTOR_FORWARD = 1,
    MOTOR_BACKWARD = 2
} motor_direction_t;

typedef struct {
    int pin_in1;
    int pin_in2;
    int pin_enable;
    int pwm_chip;
    int pwm_channel;
    int use_pwm;
    int is_initialized;
} motor_t;

/* DESPUÉS las funciones que usan motor_t */
int motorInit(motor_t* motor, int in1, int in2, int enable);
int motorSetDirection(motor_t* motor, motor_direction_t direction);
int motorSetSpeed(motor_t* motor, int speed_percent);
int motorStop(motor_t* motor);



#ifdef __cplusplus
}
#endif