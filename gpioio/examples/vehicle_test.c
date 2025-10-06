#include "gpioio.h"
#include <stdio.h>
#include <unistd.h>

// Pines motores
#define MOTOR_L_IN1  18
#define MOTOR_L_IN2  19
#define MOTOR_L_ENA  12
#define MOTOR_R_IN3  20
#define MOTOR_R_IN4  21
#define MOTOR_R_ENB  13

// Pines LEDs
#define LED_FRONT_L  23
#define LED_FRONT_R  24
#define LED_BACK_L   25
#define LED_BACK_R   8

int main(void) {
    printf("=== Prueba completa del vehículo ===\n");
    
    vehicle_leds_t leds;
    motor_t motorL, motorR;
    vehicle_t vehicle;
    
    // Inicializa LEDs
    if (ledsInit(&leds, LED_FRONT_L, LED_FRONT_R, LED_BACK_L, LED_BACK_R) < 0) {
        perror("Error inicializando LEDs");
        return 1;
    }
    
    // Inicializa motores
    if (motorInit(&motorL, MOTOR_L_IN1, MOTOR_L_IN2, MOTOR_L_ENA, &leds) < 0 ||
        motorInit(&motorR, MOTOR_R_IN3, MOTOR_R_IN4, MOTOR_R_ENB, &leds) < 0) {
        perror("Error inicializando motores");
        return 1;
    }
    
    // Inicializa vehículo
    vehicleInit(&vehicle, &motorL, &motorR, &leds);
    
    printf("Sistema inicializado\n\n");
    
    // Prueba secuencia
    printf("Adelante...\n");
    vehicleForward(&vehicle, 80);
    sleep(3);
    
    printf("Detenido...\n");
    vehicleStop(&vehicle);
    sleep(1);
    
    printf("Atrás...\n");
    vehicleBackward(&vehicle, 80);
    sleep(3);
    
    vehicleStop(&vehicle);
    sleep(1);
    
    printf("Izquierda...\n");
    vehicleLeft(&vehicle, 80);
    sleep(2);
    
    vehicleStop(&vehicle);
    sleep(1);
    
    printf("Derecha...\n");
    vehicleRight(&vehicle, 80);
    sleep(2);
    
    vehicleStop(&vehicle);
    
    printf("\n¡Prueba completada!\n");
    return 0;
}