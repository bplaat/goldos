#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>
#include <avr/pgmspace.h>

#define SERIAL_INPUT_BUFFER_SIZE 32
uint8_t serial_input_buffer[SERIAL_INPUT_BUFFER_SIZE];
uint8_t serial_input_read_position;
uint8_t serial_input_write_position;

void serial_begin(void);

uint8_t serial_available(void);

uint8_t serial_read(void);

void serial_write(uint8_t character);

void serial_print(uint8_t* string);

void serial_print_progmem(const uint8_t* string);

void serial_println(uint8_t* string);

void serial_println_progmem(const uint8_t* string);

#endif
