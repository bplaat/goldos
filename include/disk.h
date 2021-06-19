#ifndef DISK_H
#define DISK_H

#include <stdint.h>
#include <stdbool.h>

#define DISK_BLOCK_ALIGN 8

#define DISK_HEADER_SIZE DISK_BLOCK_ALIGN
#define DISK_HEADER_SIGNATURE 0
#define DISK_HEADER_VERSION 3
#define DISK_HEADER_ROOT_DIRECTORY_ADDRESS 4
#define DISK_HEADER_ROOT_DIRECTORY_SIZE 6

uint16_t disk_alloc(uint16_t size);

void disk_free(uint16_t address);

uint16_t disk_realloc(uint16_t address, uint16_t size);

void disk_format(void);

bool disk_inspect(void);

#endif
