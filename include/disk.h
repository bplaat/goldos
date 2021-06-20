#ifndef DISK_H
#define DISK_H

#include <stdint.h>

#define DISK_BLOCK_ALIGN 8

#define DISK_HEADER_SIZE DISK_BLOCK_ALIGN
#define DISK_HEADER_SIGNATURE 0
#define DISK_HEADER_VERSION 7

uint16_t disk_alloc(uint16_t size);

void disk_free(uint16_t address);

void disk_format(void);

void disk_inspect(void);

#endif
