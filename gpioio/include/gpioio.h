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

/* LEDs del vehículo */
typedef struct {
    int led_front_left;
    int led_front_right;
    int led_back_left;
    int led_back_right;
    int is_initialized;
} vehicle_leds_t;

int ledsInit(vehicle_leds_t* leds, int front_left, int front_right, 
             int back_left, int back_right);
int ledSet(int gpio, int state);
int ledsFrontOn(vehicle_leds_t* leds);
int ledsBackOn(vehicle_leds_t* leds);
int ledsLeftOn(vehicle_leds_t* leds);
int ledsRightOn(vehicle_leds_t* leds);
int ledsAllOff(vehicle_leds_t* leds);

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
    vehicle_leds_t* leds;  
} motor_t;

int motorInit(motor_t* motor, int in1, int in2, int enable, vehicle_leds_t* leds);
int motorSetDirection(motor_t* motor, motor_direction_t direction);
int motorSetSpeed(motor_t* motor, int speed_percent);
int motorStop(motor_t* motor);

// Función de alto nivel: combina dos motores + LEDs
typedef struct {
    motor_t* motor_left;
    motor_t* motor_right;
    vehicle_leds_t* leds;
} vehicle_t;

int vehicleInit(vehicle_t* vehicle, motor_t* left, motor_t* right, vehicle_leds_t* leds);
int vehicleForward(vehicle_t* vehicle, int speed);
int vehicleBackward(vehicle_t* vehicle, int speed);
int vehicleLeft(vehicle_t* vehicle, int speed);
int vehicleRight(vehicle_t* vehicle, int speed);
int vehicleStop(vehicle_t* vehicle);

int ultrasonicRead(int trigger_pin, int echo_pin);  // Retorna distancia en cm

#ifdef __cplusplus
}
#endif