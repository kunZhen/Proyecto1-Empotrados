#include "gpioio.h"
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#define MOTOR_A_IN1  18
#define MOTOR_A_IN2  19
#define MOTOR_A_ENA  12
#define MOTOR_B_IN3  20
#define MOTOR_B_IN4  21
#define MOTOR_B_ENB  13

int main(void) {
    printf("=== Prueba de Control de Motores ===\n");
    
    motor_t motorA, motorB;
    
    // Inicializa motores usando tu API
    if (motorInit(&motorA, MOTOR_A_IN1, MOTOR_A_IN2, MOTOR_A_ENA) < 0) {
        perror("Error inicializando motor A");
        return 1;
    }
    
    if (motorInit(&motorB, MOTOR_B_IN3, MOTOR_B_IN4, MOTOR_B_ENB) < 0) {
        perror("Error inicializando motor B");
        return 1;
    }
    
    printf("Motores inicializados\n\n");

    // Después de inicializar motores...

    printf("Probando velocidades...\n");
    for (int speed = 0; speed <= 100; speed += 25) {
        printf("Velocidad: %d%%\n", speed);
        motorSetDirection(&motorA, MOTOR_FORWARD);
        motorSetDirection(&motorB, MOTOR_FORWARD);
        motorSetSpeed(&motorA, speed);
        motorSetSpeed(&motorB, speed);
        sleep(2);
    }
    motorStop(&motorA);
    motorStop(&motorB);
    
    // Adelante
    printf("Adelante...\n");
    motorSetDirection(&motorA, MOTOR_FORWARD);
    motorSetDirection(&motorB, MOTOR_FORWARD);
    motorSetSpeed(&motorA, 100);
    motorSetSpeed(&motorB, 100);
    sleep(2);
    
    // Stop
    printf("Stop...\n");
    motorStop(&motorA);
    motorStop(&motorB);
    sleep(1);
    
    // Atrás
    printf("Atrás...\n");
    motorSetDirection(&motorA, MOTOR_BACKWARD);
    motorSetDirection(&motorB, MOTOR_BACKWARD);
    motorSetSpeed(&motorA, 100);
    motorSetSpeed(&motorB, 100);
    sleep(2);
    
    // Stop
    motorStop(&motorA);
    motorStop(&motorB);
    sleep(1);
    
    // Giro izquierda (motor A solo)
    printf("Izquierda...\n");
    motorSetDirection(&motorA, MOTOR_FORWARD);
    motorSetSpeed(&motorA, 100);
    sleep(2);
    
    motorStop(&motorA);
    sleep(1);
    
    // Giro derecha (motor B solo)
    printf("Derecha...\n");
    motorSetDirection(&motorB, MOTOR_FORWARD);
    motorSetSpeed(&motorB, 100);
    sleep(2);
    
    motorStop(&motorB);
    
    printf("\n¡Prueba completada!\n");
    return 0;
}