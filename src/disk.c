#include "disk.h"
// #include <stdio.h>
#include <stdbool.h>
#include "eeprom.h"
#include "utils.h"
#include "serial.h"

uint16_t disk_alloc(uint16_t size) {
    size = align(size > 0 ? size : 1, DISK_BLOCK_ALIGN);
    uint16_t block_address = DISK_BLOCK_ALIGN;
    while (block_address <= EEPROM_SIZE - 2 - 2) {
        uint16_t block_header = eeprom_read_word(block_address);
        uint16_t block_size = block_header & 0x7fff;
        if ((block_header & 0x8000) == 0 && (block_size == size || block_size - size >= 2 + 2)) {
            eeprom_write_word(block_address, 0x8000 | size);
            eeprom_write_word(block_address + 2 + size, 0x8000 | size);
            if (block_size != size) {
                uint16_t new_next_block_address = block_address + 2 + size + 2;
                uint16_t new_next_block_size = block_size - 2 - size - 2;
                eeprom_write_word(new_next_block_address, new_next_block_size);
                eeprom_write_word(new_next_block_address + 2 + new_next_block_size, new_next_block_size);
            }
            return block_address + 2;
        }
        block_address += 2 + block_size + 2;
    }
    return 0;
}

void disk_free(uint16_t address) {
    if (address == 0) return;
    uint16_t block_address = address - 2;
    uint16_t block_header = eeprom_read_word(block_address);
    if ((block_header & 0x8000) == 0) return;
    uint16_t block_size = block_header & 0x7fff;

    bool is_previous_block_free = false;
    uint16_t previous_block_address = 0;
    uint16_t previous_block_size = 0;
    if (block_address >= DISK_BLOCK_ALIGN + 2 + 2) {
        block_header = eeprom_read_word(block_address - 2);
        is_previous_block_free = (block_header & 0x8000) == 0;
        previous_block_size = block_header & 0x7fff;
        previous_block_address = block_address - 2 - previous_block_size - 2;
    }

    bool is_next_block_free = false;
    uint16_t next_block_address = block_address + 2 + block_size + 2;
    uint16_t next_block_size = 0;
    if (next_block_address <= EEPROM_SIZE - 2 - 2) {
        block_header = eeprom_read_word(next_block_address);
        is_next_block_free = (block_header & 0x8000) == 0;
        next_block_size = block_header & 0x7fff;
    }

    if (is_previous_block_free && !is_next_block_free) {
        uint16_t new_block_size = previous_block_size + 2 + 2 + block_size;
        eeprom_write_word(previous_block_address, new_block_size);
        eeprom_write_word(block_address + 2 + block_size, new_block_size);
    }
    else if (!is_previous_block_free && is_next_block_free) {
        uint16_t new_block_size = block_size + 2 + 2 + next_block_size;
        eeprom_write_word(block_address, new_block_size);
        eeprom_write_word(next_block_address + 2 + next_block_size, new_block_size);
    }
    else if (is_previous_block_free && is_next_block_free) {
        uint16_t new_block_size = previous_block_size + 2 + 2 + block_size + 2 + 2 + next_block_size;
        eeprom_write_word(previous_block_address, new_block_size);
        eeprom_write_word(next_block_address + 2 + next_block_size, new_block_size);
    }
    else {
        eeprom_write_word(block_address, block_size);
        eeprom_write_word(block_address + 2 + block_size, block_size);
    }
}

uint16_t disk_realloc(uint16_t address, uint16_t size) {
    if (address == 0) return 0;
    uint16_t block_header = eeprom_read_word(address - 2);
    if ((block_header & 0x8000) == 0) return 0;
    uint16_t block_size = block_header & 0x7fff;

    size = align(size > 0 ? size : 1, DISK_BLOCK_ALIGN);
    if (size == block_size) return address;

    disk_free(address);
    uint16_t new_address = disk_alloc(size);
    if (new_address != 0) {
        for (uint16_t i = 0; i < block_size; i++) {
            eeprom_write_byte(new_address + i, eeprom_read_byte(address + i));
        }
    }
    return new_address;
}

void disk_format(void) {
    #ifdef DEBUG
        for (uint16_t i = 0; i < EEPROM_SIZE; i++) {
            eeprom_write_word(i, 0);
        }
    #endif

    eeprom_write_byte(DISK_HEADER_SIGNATURE, 'G');
    eeprom_write_byte(DISK_HEADER_SIGNATURE + 1, 'F');
    eeprom_write_byte(DISK_HEADER_SIGNATURE + 2, 'S');
    eeprom_write_byte(DISK_HEADER_VERSION, (VERSION_MAJOR << 4) | VERSION_MINOR);

    uint16_t free_block_size = EEPROM_SIZE - DISK_HEADER_SIZE - 2 - 2;
    eeprom_write_word(DISK_HEADER_SIZE, free_block_size);
    eeprom_write_word(EEPROM_SIZE - 2, free_block_size);

    uint16_t root_directory_size = 1;
    uint16_t root_directory_address = disk_alloc(root_directory_size);
    eeprom_write_byte(root_directory_address, 0);
    eeprom_write_word(DISK_HEADER_ROOT_DIRECTORY_ADDRESS, root_directory_address);
    eeprom_write_word(DISK_HEADER_ROOT_DIRECTORY_SIZE, root_directory_size);
}

bool disk_inspect(void) {
    serial_println_P(PSTR("Disk blocks:"));

    uint16_t block_address = DISK_BLOCK_ALIGN;
    uint16_t free_block_count = 0;
    uint16_t free_blocks_size = 0;
    uint16_t max_free_block_size = 0;
    while (EEPROM_SIZE - block_address >= 2 + 2) {
        uint16_t block_header = eeprom_read_word(block_address);
        uint16_t block_size = block_header & 0x7fff;

        serial_print_P(PSTR("- "));
        serial_print_number(block_address + 2, '\0');

        if ((block_header & 0x8000) == 0) {
            serial_print_P(PSTR(": Free block of "));
            serial_print_number(block_size, '\0');
            serial_println_P(PSTR(" bytes"));

            free_block_count++;
            free_blocks_size += block_size;
            if (block_size > max_free_block_size) {
                max_free_block_size = block_size;
            }
        } else {
            serial_print_P(PSTR(": Allocated block of "));
            serial_print_number(block_size, '\0');
            serial_println_P(PSTR(" bytes"));
        }

        block_address += 2 + block_size + 2;
    }

    serial_print_P(PSTR("\nFree blocks size is "));
    serial_print_number(free_blocks_size, '\0');
    serial_print_P(PSTR(" bytes\nSeperated over "));
    serial_print_number(free_block_count, '\0');
    serial_print_P(PSTR(" free blocks\nLargest free block is "));
    serial_print_number(max_free_block_size, '\0');
    serial_println_P(PSTR(" bytes"));

    // printf("%d %d\n", block_address, EEPROM_SIZE);
    if (block_address != EEPROM_SIZE) {
        // serial_println_P(PSTR("##########################"));
        // serial_println_P(PSTR("####### DISK ERROR #######"));
        // serial_println_P(PSTR("##########################"));
        return false;
    }
    return true;
}
