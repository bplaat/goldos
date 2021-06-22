#include "goldos-dev.h"

void main(void) {
    for (int8_t i = 0; i < atoi("5"); i++) {
        serial_print_P(PSTR("Hello "));
        if (i % 2) {
            serial_println("even!");
        } else {
            serial_println("odd!");
        }
    }
}
