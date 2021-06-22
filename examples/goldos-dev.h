#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdint.h>

// Serial library
inline void serial_write(char character) {
    USIDR = character;
}

void serial_print(char *string) {
    while (*string != '\0') {
        serial_write(*string);
        string++;
    }
}

void serial_print_P(const char *string) {
    char character;
    while ((character = pgm_read_byte(string)) != '\0') {
        serial_write(character);
        string++;
    }
}

void serial_println(char *string) {
    serial_print(string);
    serial_write('\n');
}

void serial_println_P(const char *string) {
    serial_print_P(string);
    serial_write('\n');
}
