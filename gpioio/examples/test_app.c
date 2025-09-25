#include "gpioio.h"
#include <stdio.h>
#include <errno.h>

int main(void) {
    int out1 = 17;   // GPIO17 (BCM) -> salida fija a 1
    int out2 = 27;   // GPIO27 (BCM) -> blink 5 Hz por 5 s
    int in   = 22;   // GPIO22 (BCM) -> entrada

    if (pinMode(out1, OUTPUT) < 0 || pinMode(out2, OUTPUT) < 0 || pinMode(in, INPUT) < 0) {
        perror("pinMode");
        return 1;
    }

    if (digitalWrite(out1, 1) < 0) {
        perror("digitalWrite(out1)");
        return 1;
    }

    if (blink(out2, 5.0, 5.0) < 0) {
        perror("blink(out2)");
        return 1;
    }

    int v = digitalRead(in);
    if (v < 0) {
        perror("digitalRead(in)");
        return 1;
    }
    printf("GPIO %d read: %d\n", in, v);
    return 0;
}
