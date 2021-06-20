#include "file.h"
#include "disk.h"
#include "eeprom.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

File files[FILE_SIZE];

int8_t file_open(char *name, uint8_t mode) {
    uint16_t rootDirectoryAddress = eeprom_read_word(DISK_HEADER_ROOT_DIRECTORY_ADDRESS);
    uint16_t rootDirectorySize = eeprom_read_word(DISK_HEADER_ROOT_DIRECTORY_SIZE);

    uint8_t fileNameSize = strlen(name) & 0x7f;

    uint8_t numberOfFiles = eeprom_read_byte(rootDirectoryAddress);

    uint16_t position = rootDirectoryAddress + 1;
    for (uint8_t i = 0; i < numberOfFiles; i++) {
        uint8_t otherFileNameSize = eeprom_read_byte(position) & 0x7f;
        char otherFileName[128];
        for (uint8_t j = 0; j < otherFileNameSize; j++) {
            otherFileName[j] = eeprom_read_byte(position + 1 + j);
        }
        otherFileName[otherFileNameSize] = '\0';

        if (!strcmp(name, otherFileName)) {
            for (int8_t i = 0; i < FILE_SIZE; i++) {
                if (files[i].name == NULL) {
                    files[i].name = name;
                    files[i].entry = position;
                    files[i].address = eeprom_read_word(position + 1 + fileNameSize);

                    if (mode == FILE_OPEN_MODE_READ) {
                        files[i].size = eeprom_read_word(position + 1 + fileNameSize + 2);
                        files[i].position = 0;
                    }

                    if (mode == FILE_OPEN_MODE_WRITE) {
                        files[i].size = 0;
                        files[i].position = 0;

                        uint16_t new_address = disk_realloc(files[i].address, files[i].size);
                        if (new_address == 0) return -1;
                        files[i].address = new_address;

                        uint16_t entry_data_address = files[i].entry + 1 + eeprom_read_byte(files[i].entry);
                        eeprom_write_word(entry_data_address, files[i].address);
                        eeprom_write_word(entry_data_address + 2, files[i].size);
                    }

                    if (mode == FILE_OPEN_MODE_APPEND) {
                        files[i].size = eeprom_read_word(position + 1 + fileNameSize + 2);
                        files[i].position = files[i].size;
                    }

                    return i;
                }
            }
        }

        position += 1 + otherFileNameSize + 2 + 2;
    }

    if (mode == FILE_OPEN_MODE_WRITE || mode == FILE_OPEN_MODE_APPEND) {
        // Realloc root directory block for new file entry
        rootDirectorySize += 1 + fileNameSize + 2 + 2;
        rootDirectoryAddress = disk_realloc(rootDirectoryAddress, rootDirectorySize);
        if (rootDirectoryAddress == 0) {
            return -1;
        }

        eeprom_write_word(DISK_HEADER_ROOT_DIRECTORY_ADDRESS, rootDirectoryAddress);
        eeprom_write_word(DISK_HEADER_ROOT_DIRECTORY_SIZE, rootDirectorySize);

        numberOfFiles += 1;
        eeprom_write_word(rootDirectoryAddress, numberOfFiles);

        // Write file entry
        uint16_t blockAddress = disk_alloc(DISK_BLOCK_ALIGN);
        if (blockAddress == 0) {
            exit(1);
        }

        uint16_t fileEntryAddress = position;
        eeprom_write_byte(fileEntryAddress, fileNameSize);
        for (uint8_t i = 0; i < fileNameSize; i++) {
            eeprom_write_byte(fileEntryAddress + 1 + i, name[i]);
        }
        eeprom_write_word(fileEntryAddress + 1 + fileNameSize, blockAddress);
        eeprom_write_word(fileEntryAddress + 1 + fileNameSize + 2, 0);

        for (int8_t i = 0; i < FILE_SIZE; i++) {
            if (files[i].name == NULL) {
                files[i].name = name;
                files[i].size = 0;
                files[i].position = 0;
                files[i].entry = fileEntryAddress;
                files[i].address = blockAddress;
                return i;
            }
        }
    }

    return -1;
}

char *file_name(int8_t file) {
    if (file >= 0 && file < FILE_SIZE && files[file].name != NULL) {
        return files[file].name;
    }
    return NULL;
}

int16_t file_size(int8_t file) {
    if (file >= 0 && file < FILE_SIZE && files[file].name != NULL) {
        return files[file].size;
    }
    return -1;
}

int16_t file_position(int8_t file) {
    if (file >= 0 && file < FILE_SIZE && files[file].name != NULL) {
        return files[file].position;
    }
    return -1;
}

bool file_seek(int8_t file, int16_t position) {
    if (file >= 0 && file < FILE_SIZE && files[file].name != NULL) {
        files[file].position = position;
        return true;
    }
    return false;
}

int16_t file_read(int8_t file, uint8_t *buffer, int16_t size) {
    if (file >= 0 && file < FILE_SIZE && files[file].name != NULL) {
        int16_t bytes_read = 0;
        while (bytes_read < size && files[file].position < files[file].size) {
            buffer[bytes_read++] = eeprom_read_byte(files[file].address + files[file].position++);
        }
        return bytes_read;
    }
    return -1;
}

int16_t file_write(int8_t file, uint8_t *buffer, int16_t size) {
    if (file >= 0 && file < FILE_SIZE && files[file].name != NULL) {
        if (size == -1) size = strlen((char *)buffer);

        files[file].size += size;
        uint16_t new_address = disk_realloc(files[file].address, files[file].size);
        if (new_address == 0) return -1;
        files[file].address = new_address;

        int16_t bytes_writen = 0;
        while (bytes_writen < size) {
            eeprom_write_byte(files[file].address + files[file].position++, buffer[bytes_writen++]);
        }

        uint16_t entry_data_address = files[file].entry + 1 + eeprom_read_byte(files[file].entry);
        eeprom_write_word(entry_data_address, files[file].address);
        eeprom_write_word(entry_data_address + 2, files[file].size);

        return bytes_writen;
    }
    return -1;
}

bool file_close(int8_t file) {
    if (file >= 0 && file < FILE_SIZE && files[file].name != NULL) {
        files[file].name = NULL;
        return true;
    }
    return false;
}
