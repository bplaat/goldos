#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>
#ifdef ARDUINO
    #include <avr/pgmspace.h>
#else
    #define PSTR(string) (string)
    #define PROGMEM
#endif

#define SERIAL_INPUT_BUFFER_SIZE 32

extern char serial_input_buffer[];

extern uint8_t serial_input_read_position;

extern uint8_t serial_input_write_position;

void serial_begin(void);

#ifndef ARDUINO
    void serial_read_input(void);
#endif

uint8_t serial_available(void);

char serial_read(void);

void serial_write(char character);

void serial_print(char *string);

#ifdef ARDUINO
    void serial_print_progmem(const char *string);
#else
    #define serial_print_progmem(string) (serial_print((char *)string))
#endif

void serial_println(char *string);

#ifdef ARDUINO
    void serial_println_progmem(const char *string);
#else
    #define serial_println_progmem(string) (serial_println((char *)string))
#endif

#endif
