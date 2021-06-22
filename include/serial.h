#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>
#include <stdbool.h>
#ifdef ARDUINO
    #include <avr/pgmspace.h>
    #define PRIpstr "S"
#else
    #define PSTR(string) (string)
    #define PROGMEM
    #define pgm_read_byte(ptr) (*(ptr))
    #define pgm_read_word(ptr) (*(ptr))
    #define strcmp_P(s1, s2) (strcmp((s1), (s2)))
    #define printf_P(...) printf(__VA_ARGS__)
    #define PRIpstr "s"
#endif

#define SERIAL_INPUT_BUFFER_SIZE 16

extern char serial_input_buffer[];

extern uint8_t serial_input_read_position;

extern uint8_t serial_input_write_position;

void serial_begin(void);

#ifndef ARDUINO
    void serial_read_input(void);
#endif

uint8_t serial_available(void);

char serial_read(void);

void serial_read_line(char *buffer, uint8_t *size, uint8_t max_size);

void serial_write(char character);

void serial_print(char *string);

#ifdef ARDUINO
    void serial_print_P(const char *string);
#else
    #define serial_print_P(string) (serial_print((char *)(string)))
#endif

void serial_println(char *string);

#ifdef ARDUINO
    void serial_println_P(const char *string);
#else
    #define serial_println_P(string) (serial_println((char *)(string)))
#endif

void serial_print_number(int16_t number, char padding);

void serial_print_byte(uint8_t byte, char padding);

void serial_print_word(uint16_t word, char padding);

void serial_print_memory(uint8_t *data, uint16_t size);

#endif
