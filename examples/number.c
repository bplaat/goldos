#include "goldos-dev.h"

void main(void) {
    char number_buffer[7];
    itoa(12345, number_buffer, 10);
    serial_println(number_buffer);
}
