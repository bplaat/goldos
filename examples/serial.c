#include "goldos-dev.h"

void main(void) {
    serial_write('#');

    serial_print("Hoi");
    serial_println(" Jan!");

    serial_print_P(PSTR("Hoi"));
    serial_println_P(PSTR(" Jan!"));
}
