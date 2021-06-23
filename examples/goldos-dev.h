#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// Serial API

extern void serial_write(char character);

extern void serial_print(char *string);

extern void serial_print_P(const char *string);

extern void serial_println(char *string);

extern void serial_println_P(const char *string);

// File API

#define FILE_OPEN_MODE_READ 0
#define FILE_OPEN_MODE_WRITE 1
#define FILE_OPEN_MODE_APPEND 2

extern int8_t file_open(char *name, uint8_t mode);

extern bool file_name(int8_t file, char *buffer);

extern int16_t file_size(int8_t file);

extern int16_t file_position(int8_t file);

extern bool file_seek(int8_t file, int16_t position);

extern int16_t file_read(int8_t file, uint8_t *buffer, int16_t size);

extern int16_t file_write(int8_t file, uint8_t *buffer, int16_t size);

extern bool file_close(int8_t file);
