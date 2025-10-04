#pragma once
#ifdef __cplusplus
extern "C" {
#endif

typedef enum { INPUT = 0, OUTPUT = 1 } pinmode_t;

/* Exporta el GPIO a sysfs (no es obligatorio llamarla directo: pinMode la invoca). */
int gpioExport(int gpio);
/* Quita el GPIO de sysfs (opcional). */
int gpioUnexport(int gpio);
/* Activa lógica invertida (0/1) si lo necesitas; normalmente 0. */
int gpioSetActiveLow(int gpio, int active_low);

/* API pedida */
int pinMode(int gpio, pinmode_t mode);      /* direction: in/out */
int digitalWrite(int gpio, int value);      /* 0 o 1 (para OUTPUT) */
int digitalRead(int gpio);                  /* retorna 0/1, -1 en error */
int blink(int gpio, double freq_hz, double duration_sec);

#ifdef __cplusplus
}
#endif

typedef enum {
    MOTOR_STOP = 0,
    MOTOR_FORWARD = 1,
    MOTOR_BACKWARD = 2
} motor_direction_t;

typedef struct {
    int pin_in1;    // Pin dirección 1
    int pin_in2;    // Pin dirección 2  
    int pin_enable; // Pin PWM (ENA/ENB)
    int is_initialized;
} motor_t;

// Funciones para motores
int motorInit(motor_t* motor, int in1, int in2, int enable);
int motorSetDirection(motor_t* motor, motor_direction_t direction);
int motorSetSpeed(motor_t* motor, int speed_percent); // 0-100%
int motorStop(motor_t* motor);

