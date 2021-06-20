#include "commands.h"
#ifdef ARDUINO
    #include <avr/wdt.h>
#endif
#include <stdlib.h>
#include <string.h>
#include "serial.h"
#include "eeprom.h"
#include "disk.h"
// #include "file.h"
#include "utils.h"
#include "stack.h"
#include "heap.h"

const PROGMEM char rand_command_name[] = "rand";
const PROGMEM char random_command_name[] = "random";
const PROGMEM char sum_command_name[] = "sum";
const PROGMEM char average_command_name[] = "average";
const PROGMEM char hello_command_name[] = "hello";
const PROGMEM char help_command_name[] = "help";
const PROGMEM char q_command_name[] = "q";
const PROGMEM char exit_command_name[] = "exit";
const PROGMEM char ver_command_name[] = "ver";
const PROGMEM char version_command_name[] = "version";
const PROGMEM char cls_command_name[] = "cls";
const PROGMEM char clear_command_name[] = "clear";
const PROGMEM char pause_command_name[] = "pause";

const PROGMEM char eeprom_command_name[] = "eeprom";

const PROGMEM char format_command_name[] = "format";
const PROGMEM char df_command_name[] = "df";
const PROGMEM char inspect_command_name[] = "inspect";
const PROGMEM char alloc_command_name[] = "alloc";
const PROGMEM char free_command_name[] = "free";
const PROGMEM char realloc_command_name[] = "realloc";
const PROGMEM char test_command_name[] = "test";

// const PROGMEM char cat_command_name[] = "cat";
// const PROGMEM char read_command_name[] = "read";
// const PROGMEM char write_command_name[] = "write";
// const PROGMEM char ls_command_name[] = "ls";
// const PROGMEM char list_command_name[] = "list";

const PROGMEM char stack_command_name[] = "stack";

const PROGMEM char heap_command_name[] = "heap";

const Command commands[] PROGMEM = {
    { rand_command_name, &random_command }, { random_command_name, &random_command },
    { sum_command_name, &sum_command },
    { average_command_name, &average_command },
    { hello_command_name, &hello_command },
    { help_command_name, &help_command },
    { q_command_name, &exit_command }, { exit_command_name, &exit_command },
    { ver_command_name, &version_command }, { version_command_name, &version_command },
    { cls_command_name, &clear_command }, { clear_command_name, &clear_command },
    { pause_command_name, &pause_command },

    { eeprom_command_name, &eeprom_command },

    { format_command_name, &format_command },
    { df_command_name, &inspect_command }, { inspect_command_name, &inspect_command },
    { alloc_command_name, &alloc_command },
    { free_command_name, &free_command },
    { realloc_command_name, &realloc_command },
    { test_command_name, &test_command },

    // { cat_command_name, &read_command }, { read_command_name, &read_command },
    // { write_command_name, &write_command },
    // { ls_command_name, &list_command }, { list_command_name, &list_command }

    { stack_command_name, &stack_command },

    { heap_command_name, &heap_command }
};

// Util commands
void random_command(uint8_t argc, char **argv) {
    int16_t random_number;
    if (argc >= 3) {
        random_number = rand_int(atoi(argv[1]), atoi(argv[2]));
    } else if (argc >= 2) {
        random_number = rand_int(0, atoi(argv[1]));
    } else {
        random_number = rand();
    }
    serial_print_number(random_number, '\0');
    serial_write('\n');
}

void sum_command(uint8_t argc, char **argv) {
    if (argc >= 2) {
        int16_t sum = 0;
        for (uint8_t i = 1; i < argc; i++) {
            sum += atoi(argv[i]);
        }

        serial_print_number(sum, '\0');
        serial_write('\n');
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
        serial_print_number(sum / (argc - 1), '\0');
        serial_write('\n');
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

// EEPROM command
void eeprom_command(uint8_t argc, char **argv) {
    if (argc >= 2) {
        if (!strcmp_P(argv[1], PSTR("write")) && argc >= 3) {
            uint8_t position = 0;
            for (uint8_t i = 2; i < argc; i++) {
                char *string = argv[i];
                while (*string != '\0') {
                    eeprom_write_byte(position++, *string);
                    string++;
                }
                if (i != argc - 1) {
                    eeprom_write_byte(position++, ' ');
                }
            }
            eeprom_write_byte(position++, '\0');
        }

        if (!strcmp_P(argv[1], PSTR("read"))) {
            uint8_t position = 0;
            char character;
            while ((character = eeprom_read_byte(position++)) != '\0') {
                serial_write(character);
            }
        }

        if (!strcmp_P(argv[1], PSTR("dump")) || !strcmp_P(argv[1], PSTR("list"))) {
            eeprom_dump();
        }
    } else {
        serial_println_P(PSTR("Help: eeprom write [data]..., eeprom read, eeprom dump / list"));
    }
}

// Disk commands
void format_command(uint8_t argc, char **argv) {
    (void)argc;
    (void)argv;
    disk_format();
}

void inspect_command(uint8_t argc, char **argv) {
    (void)argc;
    (void)argv;
    disk_inspect();
}

void alloc_command(uint8_t argc, char **argv) {
    if (argc >= 3) {
        uint16_t count = atoi(argv[1]);
        uint16_t address = disk_alloc(count);
        if (address != 0) {
            for (uint16_t i = 0; i < count; i++) {
                eeprom_write_byte(address + i, argv[2][0]);
            }

            serial_print_P(PSTR("Address: "));
            serial_print_number(address, '\0');
            serial_write('\n');
        } else {
            serial_println_P(PSTR("Not enough free disk space!"));
        }

        // disk_inspect();
    }
}

void free_command(uint8_t argc, char **argv) {
    if (argc >= 2) {
        uint16_t address = atoi(argv[1]);
        disk_free(address);

        // disk_inspect();
    }
}

void realloc_command(uint8_t argc, char **argv) {
    if (argc >= 4) {
        uint16_t count = atoi(argv[2]);
        uint16_t address = disk_realloc(atoi(argv[1]), count);
        if (address != 0) {
            for (uint16_t i = 0; i < count; i++) {
                eeprom_write_byte(address + i, argv[3][0]);
            }

            serial_print_P(PSTR("Address: "));
            serial_print_number(address, '\0');
            serial_write('\n');
        } else {
            serial_println_P(PSTR("Not enough free disk space!"));
        }

        // disk_inspect();
    }
}

void test_command(uint8_t argc, char **argv) {
    uint16_t count = 32;
    if (argc >= 2) {
        count = atoi(argv[1]);
    }

    #define BLOCKS_SIZE 32
    uint16_t blocks[BLOCKS_SIZE];
    uint8_t blocks_size = 0;

    uint16_t block_address = DISK_BLOCK_ALIGN;
    while (EEPROM_SIZE - block_address >= 2 + 2) {
        uint16_t block_header = eeprom_read_word(block_address);
        uint16_t block_size = block_header & 0x7fff;
        if ((block_header & 0x8000) != 0) {
            blocks[blocks_size++] = block_address + 2;
        }
        block_address += 2 + block_size + 2;
    }

    for (uint16_t i = 0; i < count && disk_inspect(); i++) {
        if (blocks_size != BLOCKS_SIZE) {
            uint16_t size = rand_int(0, EEPROM_SIZE);
            uint16_t address = disk_alloc(size);
            if (address != 0) {
                blocks[blocks_size++] = address;
            }
        }

        if (blocks_size > 0) {
            uint8_t index = rand_int(0, blocks_size);
            if (rand_int(0, 1) == 1) {
                disk_free(blocks[index]);

                for (uint8_t i = index + 1; i < blocks_size; i++) {
                    blocks[i - 1] = blocks[i];
                }
                blocks_size--;
            } else {
                uint16_t size = rand_int(0, EEPROM_SIZE);
                blocks[index] = disk_realloc(blocks[index], size);

                if (blocks[index] == 0) {
                    for (uint8_t i = index + 1; i < blocks_size; i++) {
                        blocks[i - 1] = blocks[i];
                    }
                    blocks_size--;
                }
            }
        }
    }
}

// File commands
// void read_command(uint8_t argc, char **argv) {
//     if (argc >= 2) {
//         for (uint8_t i = 1; i < argc; i++) {
//             int8_t file = file_open(argv[i], FILE_OPEN_MODE_READ);
//             if (file != -1) {
//                 char buffer[16];
//                 while (file_read(file, (uint8_t *)buffer, sizeof(buffer)) != 0) {
//                     serial_print(buffer);
//                 }
//                 serial_write('\n');
//                 file_close(file);
//             }

//             eeprom_dump();
//             disk_inspect();
//         }
//     }
// }

// void write_command(uint8_t argc, char **argv) {
//     if (argc >= 3) {
//         int8_t file = file_open(argv[1], FILE_OPEN_MODE_WRITE);
//         if (file != -1) {
//             for (uint8_t i = 2; i < argc; i++) {
//                 file_write(file, (uint8_t *)argv[i], -1);
//                 file_write(file, (uint8_t *)" ", 1);
//             }
//             file_close(file);
//         }

//         eeprom_dump();
//         disk_inspect();
//     } else {
//         serial_println_P(PSTR("Help: write [path] [text]..."));
//     }
// }

// void list_command(uint8_t argc, char **argv) {
//     (void)argc;
//     (void)argv;
//     disk_inspect();
// }

// Stack command
void stack_command(uint8_t argc, char **argv) {
    if (argc >= 2) {
        if (!strcmp_P(argv[1], PSTR("push")) && argc >= 4) {
            if (!strcmp_P(argv[2], PSTR("byte"))) {
                stack_push_byte(strtol(argv[3], NULL, 16));
            }
            if (!strcmp_P(argv[2], PSTR("word"))) {
                stack_push_word(strtol(argv[3], NULL, 16));
            }
            if (!strcmp_P(argv[2], PSTR("dword"))) {
                stack_push_dword(strtol(argv[3], NULL, 16));
            }
            if (!strcmp_P(argv[2], PSTR("float"))) {
                stack_push_float(atof(argv[3]));
            }
            if (!strcmp_P(argv[2], PSTR("string"))) {
                stack_push_string(argv[3]);
            }
        }

        if (!strcmp_P(argv[1], PSTR("pop")) && argc >= 3) {
            if (!strcmp_P(argv[2], PSTR("byte"))) {
                uint8_t byte = stack_pop_byte();
                serial_print_P(PSTR("Byte: "));
                serial_print_byte(byte, '0');
                serial_write('\n');
            }
            if (!strcmp_P(argv[2], PSTR("word"))) {
                uint16_t word = stack_pop_word();
                serial_print_P(PSTR("Word: "));
                serial_print_word(word, '0');
                serial_write('\n');
            }
            if (!strcmp_P(argv[2], PSTR("dword")) || !strcmp_P(argv[2], PSTR("float"))) {
                uint32_t dword = stack_pop_dword();
                serial_print_P(PSTR("Dword: "));
                serial_print_word(dword >> 16, '0');
                serial_print_word(dword & 0xffff, '0');
                serial_write('\n');
            }
            if (!strcmp_P(argv[2], PSTR("string"))) {
                char string_buffer[64];
                stack_pop_string(string_buffer);
                serial_print_P(PSTR("String: "));
                serial_print(string_buffer);
                serial_write('\n');
            }
        }

        if (!strcmp_P(argv[1], PSTR("clear"))) {
            stack_clear();
        }

        if (!strcmp_P(argv[1], PSTR("dump"))) {
            serial_print_memory(stack, STACK_SIZE);
        }

        if (!strcmp_P(argv[1], PSTR("inspect")) || !strcmp_P(argv[1], PSTR("list"))) {
            stack_inspect();
        }
    } else {
        serial_println_P(PSTR("Help: stack push [type] [value], stack pop [type], stack clear, stack dump, stack inspect / list"));
    }
}

// Heap command
void heap_command(uint8_t argc, char **argv) {
    if (argc >= 2) {
        if (!strcmp_P(argv[1], PSTR("set")) && argc >= 5) {
            uint8_t id = strtol(argv[3], NULL, 16);
            if (!strcmp_P(argv[2], PSTR("byte"))) {
                heap_set_byte(id, strtol(argv[4], NULL, 16));
            }
            if (!strcmp_P(argv[2], PSTR("word"))) {
                heap_set_word(id, strtol(argv[4], NULL, 16));
            }
            if (!strcmp_P(argv[2], PSTR("dword"))) {
                heap_set_dword(id, strtol(argv[4], NULL, 16));
            }
            if (!strcmp_P(argv[2], PSTR("float"))) {
                heap_set_float(id, atof(argv[4]));
            }
            if (!strcmp_P(argv[2], PSTR("string"))) {
                heap_set_string(id, argv[4]);
            }
        }

        if (!strcmp_P(argv[1], PSTR("get")) && argc >= 4) {
            uint8_t id = strtol(argv[3], NULL, 16);
            if (!strcmp_P(argv[2], PSTR("byte"))) {
                uint8_t *byte = heap_get_byte(id);
                if (byte != NULL) {
                    serial_print_P(PSTR("Byte: "));
                    serial_print_byte(*byte, '0');
                    serial_write('\n');
                }
            }
            if (!strcmp_P(argv[2], PSTR("word"))) {
                uint16_t *word = heap_get_word(id);
                if (word != NULL) {
                    serial_print_P(PSTR("Word: "));
                    serial_print_word(*word, '0');
                    serial_write('\n');
                }
            }
            if (!strcmp_P(argv[2], PSTR("dword")) || !strcmp_P(argv[2], PSTR("float"))) {
                uint32_t *dword = heap_get_dword(id);
                if (dword != NULL) {
                    serial_print_P(PSTR("Dword: "));
                    serial_print_word(*dword >> 16, '0');
                    serial_print_word(*dword & 0xffff, '0');
                    serial_write('\n');
                }
            }
            if (!strcmp_P(argv[2], PSTR("string"))) {
                char *string = heap_get_string(id);
                if (string != NULL) {
                    serial_print_P(PSTR("String: "));
                    serial_print(string);
                    serial_write('\n');
                }
            }
        }

        if (!strcmp_P(argv[1], PSTR("clear"))) {
            heap_begin();
        }

        if (!strcmp_P(argv[1], PSTR("dump"))) {
            serial_print_memory(heap, HEAP_SIZE);
        }

        if (!strcmp_P(argv[1], PSTR("inspect")) || !strcmp_P(argv[1], PSTR("list"))) {
            heap_inspect();
        }
    } else {
        serial_println_P(PSTR("Help: heap set [type] [id] [value], heap get [type] [id], heap clear, heap dump, heap inspect / list"));
    }
}
