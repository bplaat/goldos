#ifndef FILE_H
#define FILE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct File {
    uint16_t address;
    uint8_t name_size;
    uint16_t size;
    uint16_t position;
} File;

#define FILE_SIZE 8
extern File files[];

#define FILE_OPEN_MODE_READ 0
#define FILE_OPEN_MODE_WRITE 1
#define FILE_OPEN_MODE_APPEND 2

int8_t file_open(char *name, uint8_t mode);

bool file_name(int8_t file, char *buffer);

int16_t file_size(int8_t file);

int16_t file_position(int8_t file);

bool file_seek(int8_t file, int16_t position);

int16_t file_read(int8_t file, uint8_t *buffer, int16_t size);

int16_t file_write(int8_t file, uint8_t *buffer, int16_t size);

bool file_close(int8_t file);

bool file_rename(char *old_name, char *new_name);

bool file_delete(char *name);

bool file_list(char *name, uint16_t *size);

#endif
