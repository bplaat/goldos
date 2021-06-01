#include "disk.h"
#include <stdbool.h>
#include "eeprom.h"
#include "config.h"

uint16_t disk_alloc(uint16_t size) {
    // Align all file allocations
    if (size == 0) size = 1;
    size += DISK_BLOCK_ALIGN - (size % DISK_BLOCK_ALIGN);

    // Check the last rest free block for space
    uint16_t blockHeader = eeprom_read_word(EEPROM_SIZE - 2);
    bool isBlockFree = (blockHeader & 0x8000) == 0;
    uint16_t blockAddress;
    uint16_t blockSize = blockHeader & 0x7fff;
    bool foundFreeBlock = false;
    if (isBlockFree && (blockSize == size || blockSize >= 2 + size + 2)) {
        foundFreeBlock = true;
        blockAddress = EEPROM_SIZE - 2 - blockSize - 2;
    } else {
        blockAddress = DISK_BLOCK_ALIGN;
    }

    // Loop from first to last block to find space
    while (!foundFreeBlock && EEPROM_SIZE - blockAddress >= 2 + 2) {
        blockHeader = eeprom_read_word(blockAddress);
        isBlockFree = (blockHeader & 0x8000) == 0;
        blockSize = blockHeader & 0x7fff;
        if (isBlockFree && (blockSize == size || blockSize >= 2 + size + 2)) {
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

void disk_free(uint16_t address) {
    if (address == 0) return;
    uint16_t blockAddress = address - 2;
    uint16_t blockHeader = eeprom_read_word(blockAddress);
    if ((blockHeader & 0x8000) == 0) return;
    uint16_t blockSize = blockHeader & 0x7fff;

    // Check if the previous block is free
    bool isPreviousBlockFree = false;
    uint16_t previousBlockAddress = 0;
    uint16_t previousBlockSize = 0;
    if (blockAddress >= DISK_HEADER_SIZE + 2 + 2) {
        uint16_t blockHeader = eeprom_read_word(blockAddress - 2);
        isPreviousBlockFree = (blockHeader & 0x8000) == 0;
        previousBlockSize = blockHeader & 0x7fff;
        previousBlockAddress = blockAddress - 2 - previousBlockSize - 2;
    }

    // Check if the next block is free
    bool isNextBlockFree = false;
    uint16_t nextBlockAddress = blockAddress + 2 + blockSize + 2;
    uint16_t nextBlockSize = 0;
    if (nextBlockAddress <= EEPROM_SIZE - 2 - 2) {
        uint16_t blockHeader = eeprom_read_word(nextBlockAddress);
        isNextBlockFree = (blockHeader & 0x8000) == 0;
        nextBlockSize = blockHeader & 0x7fff;
    }

    // Extend free block with the previous block
    if (isPreviousBlockFree && !isNextBlockFree) {
        uint16_t newBlockSize = previousBlockSize + 2 + 2 + blockSize;
        eeprom_write_word(previousBlockAddress, newBlockSize);
        eeprom_write_word(blockAddress + 2 + blockSize, newBlockSize);
    }

    // Extend free block with the next block
    else if (!isPreviousBlockFree && isNextBlockFree) {
        uint16_t newBlockSize = blockSize + 2 + 2 + nextBlockSize;
        eeprom_write_word(blockAddress, newBlockSize);
        eeprom_write_word(nextBlockAddress + 2 + nextBlockSize, newBlockSize);
    }

    // Extend free block with the previous and next block
    else if (isPreviousBlockFree && isNextBlockFree) {
        uint16_t newBlockSize = previousBlockSize + 2 + 2 + blockSize + 2 + 2 + nextBlockSize;
        eeprom_write_word(previousBlockAddress, newBlockSize);
        eeprom_write_word(nextBlockAddress + 2 + nextBlockSize, newBlockSize);
    }

    // Just make the current block free
    else {
        eeprom_write_word(blockAddress, blockSize);
        eeprom_write_word(blockAddress + 2 + blockSize, blockSize);
    }
}

uint16_t disk_realloc(uint16_t address, uint16_t size) {
    if (address == 0) return address;
    uint16_t blockAddress = address - 2;
    uint16_t blockHeader = eeprom_read_word(blockAddress);
    if ((blockHeader & 0x8000) == 0) return address;
    uint16_t blockSize = blockHeader & 0x7fff;

    if (size == 0) size = 1;
    size += DISK_BLOCK_ALIGN - (size % DISK_BLOCK_ALIGN);

    if (blockSize - size >= 2 + 2) {
        // Reduce allocate block
        eeprom_write_word(blockAddress, 0x8000 | size);
        eeprom_write_word(blockAddress + 2 + size, 0x8000 | size);

        // Create new free block
        uint16_t nextAddress = blockAddress + 2 + size + 2;
        blockSize -= 2 + size + 2;
        eeprom_write_word(nextAddress, blockSize);
        eeprom_write_word(nextAddress + 2 + blockSize, blockSize);

        return address;
    }

    if (size > blockSize) {
        // Check if the next block is free
        uint16_t nextBlockAddress = blockAddress + 2 + blockSize + 2;
        if (nextBlockAddress < EEPROM_SIZE - 2 - 2) {
            uint8_t blockHeader = eeprom_read_word(nextBlockAddress);
            bool isNextBlockFree = (blockHeader & 0x8000) == 0;
            uint16_t nextBlockSize = blockHeader & 0x7fff;

            // Extend allocated block with the next block
            uint16_t totalBlockSize = blockSize + 2 + 2 + nextBlockSize;
            if (isNextBlockFree && (totalBlockSize == size || totalBlockSize - size >= 2 + 2)) {
                // Extend allocate block
                eeprom_write_word(blockAddress, 0x8000 | size);
                eeprom_write_word(blockAddress + 2 + size, 0x8000 | size);

                // Check if there is space over for the a new free rest block
                if (totalBlockSize != size) {
                    // Create new free rest block
                    uint16_t nextAddress = blockAddress + 2 + size + 2;
                    totalBlockSize -= 2 + size + 2;
                    eeprom_write_word(nextAddress, totalBlockSize);
                    eeprom_write_word(nextAddress + 2 + totalBlockSize, totalBlockSize);
                }

                return address;
            }
        }

        // Not enough space: allocate new space, copy data and free old address
        uint16_t newAddress = disk_alloc(size);
        if (newAddress != 0) {
            for (uint16_t i = 0; i < blockSize; i++) {
                eeprom_write_byte(newAddress + i, eeprom_read_byte(address + i));
            }
        }
        disk_free(address);
        return newAddress;
    }

    // Block is already good size
    if (size == blockSize) {
        return address;
    }

    return 0;
}

void disk_format(void) {
    eeprom_write_byte(DISK_HEADER_SIGNATURE, 'G');
    eeprom_write_byte(DISK_HEADER_SIGNATURE + 1, 'F');
    eeprom_write_byte(DISK_HEADER_SIGNATURE + 2, 'S');
    eeprom_write_byte(DISK_HEADER_VERSION, (VERSION_MAJOR << 4) | VERSION_MINOR);

    uint16_t free_space_size = EEPROM_SIZE - DISK_HEADER_SIZE - 2 - 2;
    eeprom_write_word(DISK_HEADER_SIZE, free_space_size);
    eeprom_write_word(EEPROM_SIZE - 2, free_space_size);

    uint16_t root_directory_size = 1;
    uint16_t root_directory_address = disk_alloc(root_directory_size);
    eeprom_write_byte(root_directory_address, 0);
    eeprom_write_word(DISK_HEADER_ROOT_DIRECTORY_ADDRESS, root_directory_address);
    eeprom_write_word(DISK_HEADER_ROOT_DIRECTORY_SIZE, root_directory_size);
}
