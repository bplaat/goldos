#include "disk.h"
#include <stdbool.h>
#include "eeprom.h"

uint16_t disk_alloc(uint16_t size) {
    // Align all file allocations
    if (size == 0) size = 1;
    size += DISK_BLOCK_ALIGN - (size % DISK_BLOCK_ALIGN);

    // Check the last rest free block for space
    uint16_t blockHeader = eeprom_read_word(EEPROM_SIZE - 2);
    uint16_t blockAddress;
    uint16_t blockSize = blockHeader & 0x7fff;
    bool foundFreeBlock = false;
    if ((blockHeader & 0x8000) == 0 && (blockSize == size || blockSize >= 2 + size + 2)) {
        foundFreeBlock = true;
        blockAddress = EEPROM_SIZE - 2 - blockSize - 2;
    } else {
        blockAddress = DISK_BLOCK_ALIGN;
    }

    // Loop from first to last block to find space
    while (!foundFreeBlock && EEPROM_SIZE - blockAddress >= 2 + 2) {
        blockHeader = eeprom_read_word(blockAddress);
        blockSize = blockHeader & 0x7fff;
        if ((blockHeader & 0x8000) == 0 && (blockSize == size || blockSize >= 2 + size + 2)) {
            foundFreeBlock = true;
            break;
        }
        blockAddress += 2 + blockSize + 2;
    }

    // When a free block is found
    if (foundFreeBlock) {
        // Write new block headers
        eeprom_write_word(blockAddress, 0x8000 | size);
        eeprom_write_word(blockAddress + 2 + size, 0x8000 | size);

        // Check if there is space over for a new free block
        if (blockSize != size) {
            uint16_t nextBlockAddress = blockAddress + 2 + size + 2;
            uint16_t nextBlockSize = blockSize - 2 - size - 2;

            // Write back new free block headers
            eeprom_write_word(nextBlockAddress, nextBlockSize);
            eeprom_write_word(nextBlockAddress + 2 + nextBlockSize, nextBlockSize);
        }

        // Return data address
        return blockAddress + 2;
    }

    // Fail not enough space
    return 0;
}

void disk_free(uint16_t position) {
    (void)position;
}

uint16_t disk_realloc(uint16_t position, uint16_t size) {
    (void)position;
    (void)size;
    return 0;
}
