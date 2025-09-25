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
