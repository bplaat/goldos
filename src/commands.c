#include "commands.h"
#ifdef ARDUINO
    #include <avr/wdt.h>
#endif
#include <stdlib.h>
#include <string.h>
#include "serial.h"
#include "eeprom.h"
#include "disk.h"
#include "config.h"

const PROGMEM char sum_command_name[] = "sum";
const PROGMEM char average_command_name[] = "average";
const PROGMEM char hello_command_name[] = "hello";
const PROGMEM char help_command_name[] = "help";
const PROGMEM char exit_command_name[] = "exit";
const PROGMEM char version_command_name[] = "version";
const PROGMEM char clear_command_name[] = "clear";
const PROGMEM char pause_command_name[] = "pause";

const PROGMEM char read_command_name[] = "read";
const PROGMEM char write_command_name[] = "write";
const PROGMEM char format_command_name[] = "format";
const PROGMEM char dump_command_name[] = "dump";
const PROGMEM char inspect_command_name[] = "inspect";

const Command commands[] PROGMEM = {
    { sum_command_name, &sum_command },
    { average_command_name, &average_command },
    { hello_command_name, &hello_command },
    { help_command_name, &help_command },
    { exit_command_name, &exit_command },
    { version_command_name, &version_command },
    { clear_command_name, &clear_command },
    { pause_command_name, &pause_command },

    { read_command_name, &read_command },
    { write_command_name, &write_command },
    { format_command_name, &format_command },
    { dump_command_name, &dump_command },
    { inspect_command_name, &inspect_command }
};

void sum_command(uint8_t argc, char **argv) {
    if (argc >= 2) {
        int16_t sum = 0;
        for (uint8_t i = 1; i < argc; i++) {
            sum += atoi(argv[i]);
        }

        char number_buffer[7];
        itoa(sum, number_buffer, 10);
        serial_println(number_buffer);
    } else {
        serial_println_P(PSTR("Help: sum [number]..."));
    }
}

void average_command(uint8_t argc, char **argv) {
    if (argc >= 2) {
        int16_t sum = 0;
        for (uint8_t i = 1; i < argc; i++) {
            sum += atoi(argv[i]);
        }

        char number_buffer[7];
        itoa(sum / (argc - 1), number_buffer, 10);
        serial_println(number_buffer);
    } else {
        serial_println_P(PSTR("Help: average [number]..."));
    }
}

void hello_command(uint8_t argc, char **argv) {
    if (argc >= 2) {
        serial_print_P(PSTR("Hello "));
        for (uint8_t i = 1; i < argc; i++) {
            serial_print(argv[i]);
            if (i != argc - 1) {
                serial_write(' ');
            }
        }
        serial_println_P(PSTR("!"));
    } else {
        serial_println_P(PSTR("Hello World!"));
    }
}

void help_command(uint8_t argc, char **argv) {
    (void)argc;
    (void)argv;

    serial_println_P(PSTR("Commands:"));
    for (uint8_t i = 0; i < COMMANDS_SIZE; i++) {
        serial_print_P(PSTR("- "));
        serial_println_P((const char *)pgm_read_word(&commands[i].name));
    }
}

void exit_command(uint8_t argc, char **argv) {
    (void)argc;
    (void)argv;

    #ifdef ARDUINO
        wdt_enable(WDTO_15MS);
        for (;;);
    #else
        eeprom_end();
        exit(0);
    #endif
}

void version_command(uint8_t argc, char **argv) {
    (void)argc;
    (void)argv;
    serial_println_P(PSTR("GoldOS v" STR(VERSION_MAJOR) "." STR(VERSION_MINOR)));
}

void clear_command(uint8_t argc, char **argv) {
    (void)argc;
    (void)argv;
    serial_print_P(PSTR("\x1b[2J\x1b[;H"));
}

void pause_command(uint8_t argc, char **argv) {
    (void)argc;
    (void)argv;
    serial_print_P(PSTR("Press any key to continue..."));
    while (serial_available() == 0);
    serial_read();
    serial_write('\n');
}

void read_command(uint8_t argc, char **argv) {
    (void)argc;
    (void)argv;

    uint16_t position = 0;
    char character;
    while ((character = eeprom_read_byte(position++)) != '\0') {
        serial_write(character);
    }
}

void write_command(uint8_t argc, char **argv) {
    if (argc >= 2) {
        uint16_t position = 0;
        for (uint8_t i = 1; i < argc; i++) {
            char *argument = argv[i];
            while (*argument != '\0') {
                eeprom_write_byte(position++, *argument++);
            }
            eeprom_write_byte(position++, ' ');
        }
        eeprom_write_byte(position++, '\n');
        eeprom_write_byte(position++, '\0');
    } else {
        serial_println_P(PSTR("Help: write [text]..."));
    }
}

void format_command(uint8_t argc, char **argv) {
    (void)argc;
    (void)argv;

    disk_format();
}

void dump_command(uint8_t argc, char **argv) {
    (void)argc;
    (void)argv;

    size_t width = 16;

    serial_print_P(PSTR("     "));

    for (size_t x = 0; x < width; x++) {
        char number_buffer[3];
        itoa(x, number_buffer, 16);
        uint8_t diff = 2 - strlen(number_buffer);
        for (uint8_t i = 0; i < diff; i++) serial_write(' ');
        serial_print(number_buffer);

        if (x == 15) {
            serial_write('\t');
        } else {
            serial_write(' ');
        }
    }

    for (size_t x = 0; x < width; x++) {
        char number_buffer[3];
        itoa(x, number_buffer, 16);
        serial_print(number_buffer);

        if (x == 15) {
            serial_write('\n');
        } else {
            serial_write(' ');
        }
    }

    for (size_t y = 0; y < EEPROM_SIZE / width; y++) {
        char number_buffer[5];
        itoa(y * width, number_buffer, 16);
        uint8_t diff = 4 - strlen(number_buffer);
        for (uint8_t i = 0; i < diff; i++) serial_write('0');
        serial_print(number_buffer);
        serial_write(' ');

        for (size_t x = 0; x < width; x++) {
            char number_buffer[3];
            itoa(eeprom_read_byte(y * width + x), number_buffer, 16);
            uint8_t diff = 2 - strlen(number_buffer);
            for (uint8_t i = 0; i < diff; i++) serial_write('0');
            serial_print(number_buffer);

            if (x == 15) {
                serial_write('\t');
            } else {
                serial_write(' ');
            }
        }

        for (size_t x = 0; x < width; x++) {
            char character = eeprom_read_byte(y * width + x);
            if (character  < ' ' || character > '~') {
                character = '.';
            }

            serial_write(character);

            if (x == 15) {
                serial_write('\n');
            } else {
                serial_write(' ');
            }
        }
    }
}

void inspect_command(uint8_t argc, char **argv) {
    (void)argc;
    (void)argv;

    serial_println_P(PSTR("Disk blocks:"));

    uint16_t blockAddress = DISK_BLOCK_ALIGN;
    uint16_t freeBlockCount = 0;
    uint16_t freeBlocksSize = 0;
    uint16_t maxFreeBlockSize = 0;
    while (EEPROM_SIZE - blockAddress >= 2 + 2) {
        uint16_t blockHeader = eeprom_read_word(blockAddress);
        uint16_t blockSize = blockHeader & 0x7fff;
        if ((blockHeader & 0x8000) == 0) {
            serial_print_P(PSTR("- Free block of "));
            char number_buffer[7];
            itoa(blockSize, number_buffer, 10);
            serial_print(number_buffer);
            serial_println_P(PSTR(" bytes"));

            freeBlockCount++;
            freeBlocksSize += blockSize;
            if (blockSize > maxFreeBlockSize) {
                maxFreeBlockSize = blockSize;
            }
        } else {
            serial_print_P(PSTR("- Allocated block of "));
            char number_buffer[7];
            itoa(blockSize, number_buffer, 10);
            serial_print(number_buffer);
            serial_println_P(PSTR(" bytes"));
        }

        blockAddress += 2 + blockSize + 2;
    }

    serial_print_P(PSTR("\nFree blocks size is "));
    char number_buffer[7];
    itoa(freeBlocksSize, number_buffer, 10);
    serial_print(number_buffer);
    serial_print_P(PSTR(" bytes\nSeperated over "));
    itoa(freeBlockCount, number_buffer, 10);
    serial_print(number_buffer);
    serial_print_P(PSTR(" free blocks\nLargest free block is "));
    itoa(maxFreeBlockSize, number_buffer, 10);
    serial_print(number_buffer);
    serial_println_P(PSTR(" bytes"));
}
