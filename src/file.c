#include "file.h"
#include "disk.h"
#include "eeprom.h"
#include <stdlib.h>
#include <string.h>

File files[FILE_SIZE];

int8_t file_open(char *name, uint8_t mode) {
    uint16_t block_address = DISK_BLOCK_ALIGN;
    while (block_address <= EEPROM_SIZE - 2 - 2) {
        uint16_t real_block_address = block_address + 2;
        uint16_t block_header = eeprom_read_word(block_address);
        uint16_t block_size = block_header & 0x7fff;
        if ((block_header & 0x8000) != 0) {
            uint8_t file_name_size = eeprom_read_byte(real_block_address);
            if (file_name_size != 0) {
                char file_name[64];
                for (uint8_t i = 0; i < file_name_size; i++) {
                    file_name[i] = eeprom_read_byte(real_block_address + 1 + i);
                }
                file_name[file_name_size] = '\0';

                if (!strcmp(file_name, name)) {
                    for (int8_t i = 0; i < FILE_SIZE; i++) {
                        if (files[i].address == 0) {
                            files[i].address = real_block_address;
                            files[i].name_size = file_name_size;

                            if (mode == FILE_OPEN_MODE_READ) {
                                files[i].size = eeprom_read_word(real_block_address + 1 + file_name_size);
                                files[i].position = 0;
                            }

                            if (mode == FILE_OPEN_MODE_WRITE) {
                                files[i].size = 0;
                                files[i].position = 0;
                                eeprom_write_word(real_block_address + 1 + file_name_size, files[i].size);
                            }

                            if (mode == FILE_OPEN_MODE_APPEND) {
                                files[i].size = eeprom_read_word(real_block_address + 1 + file_name_size);
                                files[i].position = files[i].size;
                            }

                            return i;
                        }
                    }
                    return - 1;
                }
            }
        }
        block_address += 2 + block_size + 2;
    }

    if (mode == FILE_OPEN_MODE_WRITE || mode == FILE_OPEN_MODE_APPEND) {
        for (int8_t i = 0; i < FILE_SIZE; i++) {
            if (files[i].address == 0) {
                files[i].name_size = strlen(name);
                files[i].address = disk_alloc(1 + files[i].name_size + 2);
                files[i].size = 0;
                files[i].position = 0;

                eeprom_write_byte(files[i].address, files[i].name_size);
                for (uint8_t j = 0; j < files[i].name_size; j++) {
                    eeprom_write_byte(files[i].address + 1 + j, name[j]);
                }
                eeprom_write_word(files[i].address + 1 + files[i].name_size, files[i].size);

                return i;
            }
        }
    }
    return -1;
}

bool file_name(int8_t file, char *buffer) {
    if (file >= 0 && file < FILE_SIZE && files[file].address != 0) {
        for (uint8_t i = 0; i < files[file].name_size; i++) {
            buffer[i] = eeprom_read_byte(files[file].address + 1 + i);
        }
        buffer[files[file].name_size] = '\0';
        return true;
    }
    return false;
}

int16_t file_size(int8_t file) {
    if (file >= 0 && file < FILE_SIZE && files[file].address != 0) {
        return files[file].size;
    }
    return -1;
}

int16_t file_position(int8_t file) {
    if (file >= 0 && file < FILE_SIZE && files[file].address != 0) {
        return files[file].position;
    }
    return -1;
}

bool file_seek(int8_t file, int16_t position) {
    if (file >= 0 && file < FILE_SIZE && files[file].address != 0) {
        files[file].position = position;
        return true;
    }
    return false;
}

int16_t file_read(int8_t file, uint8_t *buffer, int16_t size) {
    if (file >= 0 && file < FILE_SIZE && files[file].address != 0) {
        int16_t bytes_read = 0;
        while (bytes_read < size && files[file].position < files[file].size) {
            buffer[bytes_read++] = eeprom_read_byte(files[file].address + 1 + files[file].name_size + 2 + files[file].position++);
        }
        return bytes_read;
    }
    return -1;
}

int16_t file_write(int8_t file, uint8_t *buffer, int16_t size) {
    if (file >= 0 && file < FILE_SIZE && files[file].address != 0) {
        if (size == -1) size = strlen((char *)buffer);

        uint16_t new_size = files[file].size + size;
        uint16_t block_size = eeprom_read_word(files[file].address - 2) & 0x7fff;
        if (new_size > block_size - 1 - files[file].name_size - 2) {
            uint16_t new_block_address = disk_alloc(1 + files[file].name_size + 2 + new_size);
            if (new_block_address != 0) {
                for (uint16_t i = 0; i < block_size; i++) {
                    uint8_t byte = eeprom_read_byte(files[file].address + i);
                    eeprom_write_byte(new_block_address + i, byte);
                }

                disk_free(files[file].address);
                files[file].address = new_block_address;
            } else {
                return -1;
            }
        }

        files[file].size = new_size;
        eeprom_write_word(files[file].address + 1 + files[file].name_size, files[file].size);

        int16_t bytes_writen = 0;
        while (bytes_writen < size) {
            eeprom_write_byte(files[file].address + 1 + files[file].name_size + 2 + files[file].position++, buffer[bytes_writen++]);
        }
        return bytes_writen;
    }
    return -1;
}

bool file_close(int8_t file) {
    if (file >= 0 && file < FILE_SIZE && files[file].address != 0) {
        files[file].address = 0;
        return true;
    }
    return false;
}

bool file_rename(char *old_name, char *new_name) {
    uint16_t block_address = DISK_BLOCK_ALIGN;
    while (block_address <= EEPROM_SIZE - 2 - 2) {
        uint16_t block_header = eeprom_read_word(block_address);
        uint16_t block_size = block_header & 0x7fff;
        if ((block_header & 0x8000) != 0) {
            uint16_t old_block_address = block_address + 2;
            uint8_t file_name_size = eeprom_read_byte(old_block_address);
            if (file_name_size != 0) {
                char file_name[64];
                for (uint8_t i = 0; i < file_name_size; i++) {
                    file_name[i] = eeprom_read_byte(old_block_address + 1 + i);
                }
                file_name[file_name_size] = '\0';

                uint16_t file_size = eeprom_read_word(old_block_address + 1 + file_name_size);
                if (!strcmp(file_name, old_name)) {
                    uint8_t new_file_name_size = strlen(new_name);

                    uint16_t new_block_address = disk_alloc(1 + new_file_name_size + 2 + file_size);
                    if (new_block_address != 0) {
                        for (uint16_t i = 0; i < file_size; i++) {
                            uint8_t byte = eeprom_read_byte(old_block_address + 1 + file_name_size + 2 + i);
                            eeprom_write_byte(new_block_address + 1 + new_file_name_size + 2 + i, byte);
                        }

                        eeprom_write_byte(new_block_address, new_file_name_size);
                        for (uint8_t i = 0; i < new_file_name_size; i++) {
                            eeprom_write_byte(new_block_address + 1 + i, new_name[i]);
                        }
                        eeprom_write_word(new_block_address + 1 + new_file_name_size, file_size);

                        disk_free(old_block_address);
                        return true;
                    } else {
                        return false;
                    }
                }
            }
        }
        block_address += 2 + block_size + 2;
    }
    return false;
}

bool file_delete(char *name) {
    uint16_t block_address = DISK_BLOCK_ALIGN;
    while (block_address <= EEPROM_SIZE - 2 - 2) {
        uint16_t block_header = eeprom_read_word(block_address);
        uint16_t real_block_address = block_address + 2;
        uint16_t block_size = block_header & 0x7fff;
        if ((block_header & 0x8000) != 0) {
            uint8_t file_name_size = eeprom_read_byte(real_block_address);
            if (file_name_size != 0) {
                char file_name[64];
                for (uint8_t i = 0; i < file_name_size; i++) {
                    file_name[i] = eeprom_read_byte(real_block_address + 1 + i);
                }
                file_name[file_name_size] = '\0';

                if (!strcmp(file_name, name)) {
                    disk_free(real_block_address);
                    return true;
                }
            }
        }
        block_address += 2 + block_size + 2;
    }
    return false;
}

bool file_list(char *name, uint16_t *size) {
    static uint16_t block_address = DISK_BLOCK_ALIGN;
    while (block_address <= EEPROM_SIZE - 2 - 2) {
        uint16_t block_header = eeprom_read_word(block_address);
        uint16_t real_block_address = block_address + 2;
        uint16_t block_size = block_header & 0x7fff;
        if ((block_header & 0x8000) != 0) {
            uint8_t file_name_size = eeprom_read_byte(real_block_address);
            if (file_name_size != 0) {
                for (uint8_t i = 0; i < file_name_size; i++) {
                    name[i] = eeprom_read_byte(real_block_address + 1 + i);
                }
                name[file_name_size] = '\0';
                *size = eeprom_read_word(real_block_address + 1 + file_name_size);
                block_address += 2 + block_size + 2;
                return true;
            }
        }
        block_address += 2 + block_size + 2;
    }
    block_address = DISK_BLOCK_ALIGN;
    return false;
}
