#include "gpioio.h"
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

// Definiciones de pines para L298N
#define MOTOR_A_IN1  18
#define MOTOR_A_IN2  19
#define MOTOR_A_ENA  12
#define MOTOR_B_IN3  20
#define MOTOR_B_IN4  21
#define MOTOR_B_ENB  13

// Funciones de control de motores
int motor_init(void) {
    // Configura todos los pines como salida
    if (pinMode(MOTOR_A_IN1, OUTPUT) < 0 ||
        pinMode(MOTOR_A_IN2, OUTPUT) < 0 ||
        pinMode(MOTOR_A_ENA, OUTPUT) < 0 ||
        pinMode(MOTOR_B_IN3, OUTPUT) < 0 ||
        pinMode(MOTOR_B_IN4, OUTPUT) < 0 ||
        pinMode(MOTOR_B_ENB, OUTPUT) < 0) {
        return -1;
    }
    
    // Inicializa todos en 0 (motores detenidos)
    digitalWrite(MOTOR_A_IN1, 0);
    digitalWrite(MOTOR_A_IN2, 0);
    digitalWrite(MOTOR_A_ENA, 0);
    digitalWrite(MOTOR_B_IN3, 0);
    digitalWrite(MOTOR_B_IN4, 0);
    digitalWrite(MOTOR_B_ENB, 0);
    
    return 0;
}

void motor_forward(void) {
    printf("Motores: Adelante\n");
    // Motor A adelante
    digitalWrite(MOTOR_A_IN1, 1);
    digitalWrite(MOTOR_A_IN2, 0);
    digitalWrite(MOTOR_A_ENA, 1);
    
    // Motor B adelante  
    digitalWrite(MOTOR_B_IN3, 1);
    digitalWrite(MOTOR_B_IN4, 0);
    digitalWrite(MOTOR_B_ENB, 1);
}

void motor_backward(void) {
    printf("Motores: Atrás\n");
    // Motor A atrás
    digitalWrite(MOTOR_A_IN1, 0);
    digitalWrite(MOTOR_A_IN2, 1);
    digitalWrite(MOTOR_A_ENA, 1);
    
    // Motor B atrás
    digitalWrite(MOTOR_B_IN3, 0);
    digitalWrite(MOTOR_B_IN4, 1);
    digitalWrite(MOTOR_B_ENB, 1);
}

void motor_left(void) {
    printf("Motores: Izquierda\n");
    // Motor A detenido/lento, Motor B adelante
    digitalWrite(MOTOR_A_IN1, 0);
    digitalWrite(MOTOR_A_IN2, 0);
    digitalWrite(MOTOR_A_ENA, 0);
    
    digitalWrite(MOTOR_B_IN3, 1);
    digitalWrite(MOTOR_B_IN4, 0);
    digitalWrite(MOTOR_B_ENB, 1);
}

void motor_right(void) {
    printf("Motores: Derecha\n");
    // Motor A adelante, Motor B detenido/lento
    digitalWrite(MOTOR_A_IN1, 1);
    digitalWrite(MOTOR_A_IN2, 0);
    digitalWrite(MOTOR_A_ENA, 1);
    
    digitalWrite(MOTOR_B_IN3, 0);
    digitalWrite(MOTOR_B_IN4, 0);
    digitalWrite(MOTOR_B_ENB, 0);
}

void motor_stop(void) {
    printf("Motores: Detenidos\n");
    digitalWrite(MOTOR_A_IN1, 0);
    digitalWrite(MOTOR_A_IN2, 0);
    digitalWrite(MOTOR_A_ENA, 0);
    digitalWrite(MOTOR_B_IN3, 0);
    digitalWrite(MOTOR_B_IN4, 0);
    digitalWrite(MOTOR_B_ENB, 0);
}

int main(void) {
    printf("=== Prueba de Control de Motores ===\n");
    
    if (motor_init() < 0) {
        perror("Error inicializando motores");
        return 1;
    }
    
    printf("Motores inicializados correctamente\n");
    
    // Secuencia de prueba
    printf("\nIniciando secuencia de prueba...\n");
    
    // Adelante 2 segundos
    motor_forward();
    sleep(2);
    
    // Stop 1 segundo
    motor_stop();
    sleep(1);
    
    // Atrás 2 segundos
    motor_backward();
    sleep(2);
    
    // Stop 1 segundo
    motor_stop();
    sleep(1);
    
    // Izquierda 2 segundos
    motor_left();
    sleep(2);
    
    // Stop 1 segundo
    motor_stop();
    sleep(1);
    
    // Derecha 2 segundos
    motor_right();
    sleep(2);
    
    // Stop final
    motor_stop();
    
    printf("\n¡Prueba completada!\n");
    
    return 0;
}