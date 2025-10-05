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


/* Motores globales */
static motor_t motor_izq;
static motor_t motor_der;
static int init_done = 0;

void system_init(void) {
    if (init_done) return;
    printf("[C] Inicializando sistema GPIO...\n");
    motorInit(&motor_izq, IN1_L, IN2_L, EN_L);
    motorInit(&motor_der, IN1_R, IN2_R, EN_R);
    init_done = 1;
}

/* === Funciones expuestas para Python === */

void move_forward(void) {
    system_init();
    printf("[C] Movimiento: adelante\n");
    motorSetDirection(&motor_izq, MOTOR_FORWARD);
    motorSetDirection(&motor_der, MOTOR_FORWARD);
    motorSetSpeed(&motor_izq, 100);
    motorSetSpeed(&motor_der, 100);
}

void move_backward(void) {
    system_init();
    printf("[C] Movimiento: atrás\n");
    motorSetDirection(&motor_izq, MOTOR_BACKWARD);
    motorSetDirection(&motor_der, MOTOR_BACKWARD);
    motorSetSpeed(&motor_izq, 100);
    motorSetSpeed(&motor_der, 100);
}

void turn_left(void) {
    system_init();
    printf("[C] Movimiento: izquierda\n");
    motorSetDirection(&motor_izq, MOTOR_STOP);
    motorSetDirection(&motor_der, MOTOR_FORWARD);
}

void turn_right(void) {
    system_init();
    printf("[C] Movimiento: derecha\n");
    motorSetDirection(&motor_izq, MOTOR_FORWARD);
    motorSetDirection(&motor_der, MOTOR_STOP);
}

void stop(void) {
    system_init();
    printf("[C] Movimiento: detener\n");
    motorStop(&motor_izq);
    motorStop(&motor_der);
}


static int write_str(const char *path, const char *s) {
    int fd = open(path, O_WRONLY);
    if (fd < 0) return -1;
    ssize_t n = write(fd, s, strlen(s));
    int e = (n < 0) ? -1 : 0;
    int saved = errno;
    close(fd);
    errno = saved;
    return e;
}

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

static int path_exists(const char *p) {
    struct stat st;
    return stat(p, &st) == 0;
}

/* Espera hasta ms_timeout a que exista "path" */
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

/* Detecta el base del gpiochip del SoC.
   - Si existe env GPIOIO_BASE, la usamos.
   - Si algún label contiene "pinctrl" o "bcm", usamos ese base.
   - Si no, usamos el base más pequeño disponible.
   - Si todo falla, 0. */
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

static int phys_gpio(int bcm) { return bcm + gpio_base(); }

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

int gpioSetActiveLow(int bcm, int active_low) {
    if (ensure_exported(bcm) < 0) return -1;
    int g = phys_gpio(bcm);
    char path[160], val[4];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/active_low", g);
    snprintf(val, sizeof(val), "%d", active_low ? 1 : 0);
    return write_str(path, val);
}

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

int digitalWrite(int bcm, int value) {
    int g = phys_gpio(bcm);
    char path[160];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", g);
    return write_str(path, value ? "1" : "0");
}

int digitalRead(int bcm) {
    int g = phys_gpio(bcm);
    char path[160], buf[8];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", g);
    if (read_str(path, buf, sizeof(buf)) < 0) return -1;
    return (buf[0] == '0') ? 0 : 1;
}

static void sleep_seconds(double sec) {
    struct timespec ts;
    ts.tv_sec  = (time_t)sec;
    ts.tv_nsec = (long)((sec - ts.tv_sec) * 1e9);
    if (ts.tv_nsec < 0) ts.tv_nsec = 0;
    while (nanosleep(&ts, &ts) && errno == EINTR) { }
}

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

static int pwm_export(int gpio) {
    // Intenta usar PWM hardware primero
    char path[128], num[16];
    snprintf(num, sizeof(num), "%d", gpio);
    
    // Busca PWM chip disponible para este GPIO
    for (int chip = 0; chip < 4; chip++) {
        snprintf(path, sizeof(path), "/sys/class/pwm/pwmchip%d/export", chip);
        if (write_str(path, "0") == 0) {
            return chip; // Retorna el chip PWM usado
        }
    }
    return -1; // No hay PWM hardware disponible
}

static int pwm_software(int gpio, int duty_percent, double freq_hz) {
    // PWM por software usando tu biblioteca existente
    if (pinMode(gpio, OUTPUT) < 0) return -1;
    
    double period = 1.0 / freq_hz;
    double on_time = period * (duty_percent / 100.0);
    double off_time = period - on_time;
    
    // Solo un ciclo para prueba - en aplicación real usar thread
    digitalWrite(gpio, 1);
    sleep_seconds(on_time);
    digitalWrite(gpio, 0);
    sleep_seconds(off_time);
    
    return 0;
}

int motorInit(motor_t* motor, int in1, int in2, int enable) {
    if (!motor) return -1;
    
    motor->pin_in1 = in1;
    motor->pin_in2 = in2;
    motor->pin_enable = enable;
    
    // Configura pines de dirección como salida
    if (pinMode(in1, OUTPUT) < 0 || pinMode(in2, OUTPUT) < 0) {
        return -1;
    }
    
    // Configura pin de velocidad
    if (pinMode(enable, OUTPUT) < 0) {
        return -1;
    }
    
    // Inicializa en stop
    digitalWrite(in1, 0);
    digitalWrite(in2, 0);
    digitalWrite(enable, 0);
    
    motor->is_initialized = 1;
    return 0;
}

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

int motorSetSpeed(motor_t* motor, int speed_percent) {
    if (!motor || !motor->is_initialized) return -1;
    if (speed_percent < 0 || speed_percent > 100) return -1;
    
    if (speed_percent == 0) {
        return digitalWrite(motor->pin_enable, 0);
    } else if (speed_percent == 100) {
        return digitalWrite(motor->pin_enable, 1);
    } else {
        // Para velocidades intermedias, necesitas PWM
        // Por ahora, implementación simplificada:
        return digitalWrite(motor->pin_enable, 1);
    }
}

int motorStop(motor_t* motor) {
    if (!motor || !motor->is_initialized) return -1;
    
    digitalWrite(motor->pin_in1, 0);
    digitalWrite(motor->pin_in2, 0);
    digitalWrite(motor->pin_enable, 0);
    return 0;
}