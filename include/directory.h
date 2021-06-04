#ifndef DIRECTORY_H
#define DIRECTORY_H

#include <stdint.h>
#include <stdbool.h>

typedef struct Directory {
    char* path;
    uint16_t size;
    uint16_t position;
    uint16_t entry;
    uint16_t address;
} Directory;

#define DIRECTORY_SIZE 4
extern Directory directories[];

#define DIRECTORY_OPEN_MODE_READ 0
#define DIRECTORY_OPEN_MODE_CREATE 1

int8_t directory_open(char *path, uint8_t mode);

#define DIRECTORY_READ_TYPE_FILE 0
#define DIRECTORY_READ_TYPE_DIRECTORY 1

bool directory_read(int8_t directory, uint8_t *type, char *name, uint16_t *size);

bool directory_close(int8_t directory);

#endif
