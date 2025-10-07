#define _GNU_SOURCE
#include "gpioio.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>
#include <sys/time.h>

/* ==========================
 * Utilidades de E/S en sysfs
 * ========================== */

/* Escribe una cadena 's' en el archivo 'path'.
   Devuelve 0 en éxito o -1 en error y conserva errno del write(). */
static int write_str(const char *path, const char *s) {
    int fd = open(path, O_WRONLY);
    if (fd < 0) return -1;
    ssize_t n = write(fd, s, strlen(s));
    int e = (n < 0) ? -1 : 0;
    int saved = errno; // preserva errno real del write
    close(fd);
    errno = saved;
    return e;
}

/* Lee hasta bufsz-1 bytes de 'path' y pone terminador NUL.
   Devuelve 0 en éxito o -1 en error y conserva errno del read(). */
static int read_str(const char *path, char *buf, size_t bufsz) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return -1;
    ssize_t n = read(fd, buf, bufsz - 1);
    int e = (n < 0) ? -1 : 0;
    if (n >= 0) buf[n] = '\0';
    int saved = errno;
    close(fd);
    errno = saved;
    return e;
}

/* Verifica existencia de un path (stat == 0). */
static int path_exists(const char *p) {
    struct stat st;
    return stat(p, &st) == 0;
}

/* Espera hasta ms_timeout para que exista 'path'.
   Sondea cada 10 ms. Devuelve 0 si aparece; -1 (ETIMEDOUT) si no. */
static int wait_for_path(const char *path, int ms_timeout) {
    const int step_ms = 10;
    int waited = 0;
    while (waited <= ms_timeout) {
        if (path_exists(path)) return 0;
        struct timespec ts = {0, step_ms * 1000000L};
        nanosleep(&ts, NULL);
        waited += step_ms;
    }
    errno = ETIMEDOUT;
    return -1;
}

/* ===========================================================
 * Descubrimiento del "base" de gpiochip para mapear BCM->sysfs
 * ===========================================================
 * - Usa variable de entorno GPIOIO_BASE si está presente.
 * - Si encuentra un chip con label que contenga "pinctrl" o "bcm",
 *   usa su base (suele funcionar en Raspberry Pi).
 * - Si no, elige el base más pequeño (primer gpiochip del sistema).
 * - Si nada sirve, retorna 0.
 */
static int gpio_base(void) {
    static int cached = INT_MIN;
    if (cached != INT_MIN) return cached;

    const char *e = getenv("GPIOIO_BASE");
    if (e && *e) { cached = atoi(e); return cached; }

    DIR *d = opendir("/sys/class/gpio");
    if (!d) { cached = 0; return 0; }

    int best_base_label = INT_MAX;
    int best_base_min   = INT_MAX;

    struct dirent *de;
    while ((de = readdir(d))) {
        if (strncmp(de->d_name, "gpiochip", 8) != 0) continue;

        char p[256], buf[64], label[256] = {0};
        int base = 0;

        snprintf(p, sizeof(p), "/sys/class/gpio/%s/base", de->d_name);
        if (read_str(p, buf, sizeof(buf)) < 0) continue;
        base = atoi(buf);

        snprintf(p, sizeof(p), "/sys/class/gpio/%s/label", de->d_name);
        read_str(p, label, sizeof(label));

        if (strstr(label, "pinctrl") || strstr(label, "bcm")) {
            if (base < best_base_label) best_base_label = base;
        }
        if (base < best_base_min) best_base_min = base;
    }
    closedir(d);

    if (best_base_label != INT_MAX) cached = best_base_label;
    else if (best_base_min   != INT_MAX) cached = best_base_min;
    else cached = 0;

    return cached;
}

/* Convierte un número BCM (lógico) a número físico de sysfs:
   físico = base + bcm. */
static int phys_gpio(int bcm) { return bcm + gpio_base(); }

/* Exporta el GPIO si aún no está exportado y espera a 'direction'
   para evitar carreras que causen EINVAL. */
static int ensure_exported(int bcm) {
    int g = phys_gpio(bcm);
    char dirpath[128], num[16];
    snprintf(dirpath, sizeof(dirpath), "/sys/class/gpio/gpio%d", g);
    if (path_exists(dirpath)) return 0;

    snprintf(num, sizeof(num), "%d", g);
    if (write_str("/sys/class/gpio/export", num) < 0) {
        if (errno == EBUSY) return 0;
        return -1;
    }
    /* Espera a que aparezca 'direction' para evitar EINVAL por carrera */
    char direction[160];
    snprintf(direction, sizeof(direction), "%s/direction", dirpath);
    if (wait_for_path(direction, 500) < 0) return -1;
    return 0;
}

int gpioExport(int bcm) {
    int g = phys_gpio(bcm);
    char num[16];
    snprintf(num, sizeof(num), "%d", g);
    if (write_str("/sys/class/gpio/export", num) == 0) return 0;
    return (errno == EBUSY) ? 0 : -1;
}

int gpioUnexport(int bcm) {
    int g = phys_gpio(bcm);
    char num[16];
    snprintf(num, sizeof(num), "%d", g);
    return write_str("/sys/class/gpio/unexport", num);
}

/* Configura active_low (0/1) del GPIO.
   active_low=1 invierte la lógica de value (útil con transistores). */
int gpioSetActiveLow(int bcm, int active_low) {
    if (ensure_exported(bcm) < 0) return -1;
    int g = phys_gpio(bcm);
    char path[160], val[4];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/active_low", g);
    snprintf(val, sizeof(val), "%d", active_low ? 1 : 0);
    return write_str(path, val);
}

/* pinMode estilo Arduino: OUTPUT/INPUT vía sysfs/direction.
   Algunos kernels aceptan "low"/"high" para setear estado inicial. */
int pinMode(int bcm, pinmode_t mode) {
    if (ensure_exported(bcm) < 0) return -1;
    int g = phys_gpio(bcm);
    char path[160];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", g);
    if (mode == OUTPUT) {
        if (write_str(path, "out") == 0) return 0;
        if (errno == EINVAL) { /* algunos kernels prefieren "low/high" */
            if (write_str(path, "low") == 0) return 0;
        }
        return -1;
    } else {
        return write_str(path, "in");
    }
}

/* Escribe 0/1 en value. (No verifica que sea salida.) */
int digitalWrite(int bcm, int value) {
    int g = phys_gpio(bcm);
    char path[160];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", g);
    return write_str(path, value ? "1" : "0");
}

/* Lee value y devuelve 0/1 (o -1 en error). */
int digitalRead(int bcm) {
    int g = phys_gpio(bcm);
    char path[160], buf[8];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", g);
    if (read_str(path, buf, sizeof(buf)) < 0) return -1;
    return (buf[0] == '0') ? 0 : 1;
}

/* Dormir en segundos fraccionarios con nanosleep,
   reintentando si es interrumpido por señales (EINTR). */
static void sleep_seconds(double sec) {
    struct timespec ts;
    ts.tv_sec  = (time_t)sec;
    ts.tv_nsec = (long)((sec - ts.tv_sec) * 1e9);
    if (ts.tv_nsec < 0) ts.tv_nsec = 0;
    while (nanosleep(&ts, &ts) && errno == EINTR) { }
}

/* Parpadeo básico de un GPIO como salida a 'freq_hz' durante 'duration_sec'.
   Alterna HIGH/LOW con semiperiodos y apaga al final. */
int blink(int bcm, double freq_hz, double duration_sec) {
    if (freq_hz <= 0.0 || duration_sec <= 0.0) { errno = EINVAL; return -1; }
    if (pinMode(bcm, OUTPUT) < 0) return -1;

    double half = 0.5 / freq_hz;
    double elapsed = 0.0;
    int state = 0;

    if (digitalWrite(bcm, 0) < 0) return -1;

    const double step = half;
    while (elapsed < duration_sec) {
        state = !state;
        if (digitalWrite(bcm, state) < 0) return -1;
        sleep_seconds(half);
        elapsed += step;
    }
    (void)digitalWrite(bcm, 0);
    return 0;
}

/* =========================
 * Mapeo GPIO <-> PWM sysfs
 * =========================
 * pwm_map: define qué GPIOs (BCM) tienen PWM hardware y en qué
 * pwmchip/channel caen en tu plataforma.
 * OJO: Esto varía entre placas y kernels; ajusta según tu SoC.
 */
typedef struct {
    int gpio;
    int pwm_chip;
    int pwm_channel;
} gpio_pwm_map_t;

static const gpio_pwm_map_t pwm_map[] = {
    {12, 0, 0},
    {13, 0, 1},
    {18, 0, 0},
    {19, 0, 1},
};
static const int pwm_map_size = sizeof(pwm_map) / sizeof(pwm_map[0]);

/* Busca en el mapa si 'gpio' dispone de PWM y devuelve chip/canal. */
static int gpio_to_pwm(int gpio, int *chip, int *channel) {
    for (int i = 0; i < pwm_map_size; i++) {
        if (pwm_map[i].gpio == gpio) {
            *chip = pwm_map[i].pwm_chip;
            *channel = pwm_map[i].pwm_channel;
            return 0;
        }
    }
    return -1;
}

/* Exporta un canal PWM y espera a que aparezca 'period'. */
int pwmExport(int pwm_chip, int pwm_channel) {
    char path[128], num[16];
    snprintf(path, sizeof(path), "/sys/class/pwm/pwmchip%d/export", pwm_chip);
    snprintf(num, sizeof(num), "%d", pwm_channel);
    
    if (write_str(path, num) < 0) {
        if (errno == EBUSY) return 0;
        return -1;
    }
    
    snprintf(path, sizeof(path), "/sys/class/pwm/pwmchip%d/pwm%d/period", 
             pwm_chip, pwm_channel);
    return wait_for_path(path, 500);
}

int pwmUnexport(int pwm_chip, int pwm_channel) {
    char path[128], num[16];
    snprintf(path, sizeof(path), "/sys/class/pwm/pwmchip%d/unexport", pwm_chip);
    snprintf(num, sizeof(num), "%d", pwm_channel);
    return write_str(path, num);
}

/* Ajusta periodo del PWM (nanosegundos). */
int pwmSetPeriod(int pwm_chip, int pwm_channel, unsigned long period_ns) {
    char path[128], val[32];
    snprintf(path, sizeof(path), "/sys/class/pwm/pwmchip%d/pwm%d/period", 
             pwm_chip, pwm_channel);
    snprintf(val, sizeof(val), "%lu", period_ns);
    return write_str(path, val);
}

/* Ajusta ciclo de trabajo del PWM (nanosegundos). Debe ser <= period. */
int pwmSetDutyCycle(int pwm_chip, int pwm_channel, unsigned long duty_ns) {
    char path[128], val[32];
    snprintf(path, sizeof(path), "/sys/class/pwm/pwmchip%d/pwm%d/duty_cycle", 
             pwm_chip, pwm_channel);
    snprintf(val, sizeof(val), "%lu", duty_ns);
    return write_str(path, val);
}

/* Habilita/inhabilita la salida PWM (1/0). */
int pwmEnable(int pwm_chip, int pwm_channel, int enable) {
    char path[128];
    snprintf(path, sizeof(path), "/sys/class/pwm/pwmchip%d/pwm%d/enable", 
             pwm_chip, pwm_channel);
    return write_str(path, enable ? "1" : "0");
}

/* ==========================
 *      CONTROL DE LEDs
 * ==========================
 * Abstracción para 4 LEDs (frontal izq/der y trasero izq/der).
 * Requiere que vehicle_leds_t defina esos campos + is_initialized.
 */

/* Inicializa los GPIOs de los LEDs como salidas y los apaga. */
int ledsInit(vehicle_leds_t* leds, int front_left, int front_right,
             int back_left, int back_right) {
    if (!leds) return -1;
    
    leds->led_front_left = front_left;
    leds->led_front_right = front_right;
    leds->led_back_left = back_left;
    leds->led_back_right = back_right;
    
    // Configura todos como salida
    if (pinMode(front_left, OUTPUT) < 0 ||
        pinMode(front_right, OUTPUT) < 0 ||
        pinMode(back_left, OUTPUT) < 0 ||
        pinMode(back_right, OUTPUT) < 0) {
        return -1;
    }
    
    // Apaga todos inicialmente
    digitalWrite(front_left, 0);
    digitalWrite(front_right, 0);
    digitalWrite(back_left, 0);
    digitalWrite(back_right, 0);
    
    leds->is_initialized = 1;
    return 0;
}

/* Set directo de un LED (GPIO) a 0/1. */
int ledSet(int gpio, int state) {
    return digitalWrite(gpio, state ? 1 : 0);
}

/* Enciende solo LEDs frontales (y apaga traseros). */
int ledsFrontOn(vehicle_leds_t* leds) {
    if (!leds || !leds->is_initialized) return -1;
    digitalWrite(leds->led_front_left, 1);
    digitalWrite(leds->led_front_right, 1);
    digitalWrite(leds->led_back_left, 0);
    digitalWrite(leds->led_back_right, 0);
    return 0;
}

/* Enciende solo LEDs traseros. */
int ledsBackOn(vehicle_leds_t* leds) {
    if (!leds || !leds->is_initialized) return -1;
    digitalWrite(leds->led_front_left, 0);
    digitalWrite(leds->led_front_right, 0);
    digitalWrite(leds->led_back_left, 1);
    digitalWrite(leds->led_back_right, 1);
    return 0;
}

/* Enciende lado izquierdo (frontal y trasero). */
int ledsLeftOn(vehicle_leds_t* leds) {
    if (!leds || !leds->is_initialized) return -1;
    digitalWrite(leds->led_front_left, 1);
    digitalWrite(leds->led_front_right, 0);
    digitalWrite(leds->led_back_left, 1);
    digitalWrite(leds->led_back_right, 0);
    return 0;
}

/* Enciende lado derecho (frontal y trasero). */
int ledsRightOn(vehicle_leds_t* leds) {
    if (!leds || !leds->is_initialized) return -1;
    digitalWrite(leds->led_front_left, 0);
    digitalWrite(leds->led_front_right, 1);
    digitalWrite(leds->led_back_left, 0);
    digitalWrite(leds->led_back_right, 1);
    return 0;
}

/* Apaga todos los LEDs. */
int ledsAllOff(vehicle_leds_t* leds) {
    if (!leds || !leds->is_initialized) return -1;
    digitalWrite(leds->led_front_left, 0);
    digitalWrite(leds->led_front_right, 0);
    digitalWrite(leds->led_back_left, 0);
    digitalWrite(leds->led_back_right, 0);
    return 0;
}

/* ==========================
 *   CONTROL DE MOTORES
 * ==========================
 * Driver tipo H-bridge con IN1/IN2 y ENABLE.
 * Si 'enable' coincide con un GPIO con PWM hardware, se usa PWM sysfs.
 * Si no, se usa GPIO digital on/off como fallback (velocidad binaria).
 */

/* Inicializa pines del motor y configura PWM si disponible. */
int motorInit(motor_t* motor, int in1, int in2, int enable, vehicle_leds_t* leds) {
    if (!motor) return -1;
    
    motor->pin_in1 = in1;
    motor->pin_in2 = in2;
    motor->pin_enable = enable;
    motor->use_pwm = 0;
    motor->leds = leds;  // Guarda referencia a LEDs
    
    if (pinMode(in1, OUTPUT) < 0 || pinMode(in2, OUTPUT) < 0) {
        return -1;
    }
    
    if (gpio_to_pwm(enable, &motor->pwm_chip, &motor->pwm_channel) == 0) {
        if (pwmExport(motor->pwm_chip, motor->pwm_channel) == 0) {
            pwmSetPeriod(motor->pwm_chip, motor->pwm_channel, 1000000);
            pwmSetDutyCycle(motor->pwm_chip, motor->pwm_channel, 0);
            pwmEnable(motor->pwm_chip, motor->pwm_channel, 1);
            motor->use_pwm = 1;
        }
    }
    
    if (!motor->use_pwm) {
        if (pinMode(enable, OUTPUT) < 0) {
            return -1;
        }
    }
    
    digitalWrite(in1, 0);
    digitalWrite(in2, 0);
    if (!motor->use_pwm) {
        digitalWrite(enable, 0);
    }
    
    motor->is_initialized = 1;
    return 0;
}

/* Define la dirección del motor: adelante, atrás o stop.
   Se hace con combinaciones IN1/IN2 típicas del puente H. */
int motorSetDirection(motor_t* motor, motor_direction_t direction) {
    if (!motor || !motor->is_initialized) return -1;
    
    switch (direction) {
        case MOTOR_FORWARD:
            digitalWrite(motor->pin_in1, 1);
            digitalWrite(motor->pin_in2, 0);
            break;
        case MOTOR_BACKWARD:
            digitalWrite(motor->pin_in1, 0);
            digitalWrite(motor->pin_in2, 1);
            break;
        case MOTOR_STOP:
        default:
            digitalWrite(motor->pin_in1, 0);
            digitalWrite(motor->pin_in2, 0);
            break;
    }
    return 0;
}

/* Ajusta la velocidad:
   - Con PWM: escala duty en ns respecto a periodo 1,000,000 ns (1 kHz).
   - Sin PWM: actúa como on/off (0% apagado, >0% encendido). */
int motorSetSpeed(motor_t* motor, int speed_percent) {
    if (!motor || !motor->is_initialized) return -1;
    if (speed_percent < 0 || speed_percent > 100) return -1;
    
    if (motor->use_pwm) {
        unsigned long duty = (1000000UL * speed_percent) / 100;
        return pwmSetDutyCycle(motor->pwm_chip, motor->pwm_channel, duty);
    } else {
        return digitalWrite(motor->pin_enable, speed_percent > 0 ? 1 : 0);
    }
}

/* Detiene el motor (IN1/IN2 a 0 y duty 0 si PWM). */
int motorStop(motor_t* motor) {
    if (!motor || !motor->is_initialized) return -1;
    
    digitalWrite(motor->pin_in1, 0);
    digitalWrite(motor->pin_in2, 0);
    
    if (motor->use_pwm) {
        pwmSetDutyCycle(motor->pwm_chip, motor->pwm_channel, 0);
    } else {
        digitalWrite(motor->pin_enable, 0);
    }
    return 0;
}

/* ==========================
 *     VEHÍCULO (2 MOTORES)
 * ==========================
 * Comandos compuestos: mover y sincronizar LEDs indicativos.
 */

/* Conecta referencias (no inicializa pines aquí). */
int vehicleInit(vehicle_t* vehicle, motor_t* left, motor_t* right, vehicle_leds_t* leds) {
    if (!vehicle || !left || !right || !leds) return -1;
    vehicle->motor_left = left;
    vehicle->motor_right = right;
    vehicle->leds = leds;
    return 0;
}

/* Avanza: ambos motores forward, LEDs frontales. */
int vehicleForward(vehicle_t* vehicle, int speed) {
    if (!vehicle) return -1;
    ledsFrontOn(vehicle->leds);
    motorSetDirection(vehicle->motor_left, MOTOR_FORWARD);
    motorSetDirection(vehicle->motor_right, MOTOR_FORWARD);
    motorSetSpeed(vehicle->motor_left, speed);
    motorSetSpeed(vehicle->motor_right, speed);
    return 0;
}

/* Retrocede: ambos motores backward, LEDs traseros. */
int vehicleBackward(vehicle_t* vehicle, int speed) {
    if (!vehicle) return -1;
    ledsBackOn(vehicle->leds);
    motorSetDirection(vehicle->motor_left, MOTOR_BACKWARD);
    motorSetDirection(vehicle->motor_right, MOTOR_BACKWARD);
    motorSetSpeed(vehicle->motor_left, speed);
    motorSetSpeed(vehicle->motor_right, speed);
    return 0;
}

/* Gira a la izquierda: detiene motor izquierdo y mueve derecho.
   (LEDs indican el lado opuesto encendido en tu convención.) */
int vehicleLeft(vehicle_t* vehicle, int speed) {
    if (!vehicle) return -1;
    ledsRightOn(vehicle->leds);
    motorSetDirection(vehicle->motor_left, MOTOR_STOP);
    motorSetDirection(vehicle->motor_right, MOTOR_FORWARD);
    motorSetSpeed(vehicle->motor_left, 0);
    motorSetSpeed(vehicle->motor_right, speed);
    return 0;
}

/* Gira a la derecha: motor izquierdo avanza, derecho detenido. */
int vehicleRight(vehicle_t* vehicle, int speed) {
    if (!vehicle) return -1;
    ledsLeftOn(vehicle->leds);
    motorSetDirection(vehicle->motor_left, MOTOR_FORWARD);
    motorSetDirection(vehicle->motor_right, MOTOR_STOP);
    motorSetSpeed(vehicle->motor_left, speed);
    motorSetSpeed(vehicle->motor_right, 0);
    return 0;
}

/* Frenado total: apaga LEDs y detiene ambos motores. */
int vehicleStop(vehicle_t* vehicle) {
    if (!vehicle) return -1;
    ledsAllOff(vehicle->leds);
    motorStop(vehicle->motor_left);
    motorStop(vehicle->motor_right);
    return 0;
}

/* =========================================
 *    Sensor Ultrasónico (HC-SR04 típico)
 * =========================================
 * 'trigger_pin' genera un pulso de 10 µs.
 * 'echo_pin' mide duración del pulso de retorno.
 * Retorna distancia aproximada en centímetros (cm).
 * Timeouts de 30 ms para evitar bloqueos.
 * IMPORTANTE:
 * - Requiere precisión temporal; GPIO sysfs no es ideal para µs.
 * - Evita interferencias: no uses busy-wait prolongado en sistemas cargados.
 */
int ultrasonicRead(int trigger_pin, int echo_pin) {
    struct timeval start, end;
    long elapsed_us;
    
    // Asegura que los pines estén configurados
    pinMode(trigger_pin, OUTPUT);
    pinMode(echo_pin, INPUT);
    
    // Envía pulso de trigger (10us)
    digitalWrite(trigger_pin, 0);
    usleep(2);
    digitalWrite(trigger_pin, 1);
    usleep(10);
    digitalWrite(trigger_pin, 0);
    
    // Espera a que echo sea HIGH (timeout 30ms)
    int timeout = 30000; // 30ms
    while (digitalRead(echo_pin) == 0 && timeout-- > 0) {
        usleep(1);
    }
    if (timeout <= 0) return -1;
    
    gettimeofday(&start, NULL);
    
    // Espera a que echo sea LOW (timeout 30ms)
    timeout = 30000;
    while (digitalRead(echo_pin) == 1 && timeout-- > 0) {
        usleep(1);
    }
    if (timeout <= 0) return -1;
    
    gettimeofday(&end, NULL);
    
    // Calcula distancia: tiempo (us) * velocidad sonido (343 m/s) / 2
    elapsed_us = (end.tv_sec - start.tv_sec) * 1000000 + 
                 (end.tv_usec - start.tv_usec);
    
    return (int)(elapsed_us / 58.0); // Distancia en cm
}