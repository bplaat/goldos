#include "commands.h"
#ifdef ARDUINO
    #include <avr/wdt.h>
#endif
#ifndef ARDUINO
    #include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>
#include "utils.h"
#include "serial.h"
#include "eeprom.h"
#include "disk.h"
#include "file.h"
#include "stack.h"
#include "heap.h"
#include "processor.h"

const PROGMEM char random_command_name[] = "random";
const PROGMEM char rand_command_name[] = "rand";
const PROGMEM char sum_command_name[] = "sum";
const PROGMEM char average_command_name[] = "average";
const PROGMEM char hello_command_name[] = "hello";
const PROGMEM char help_command_name[] = "help";
const PROGMEM char exit_command_name[] = "exit";
const PROGMEM char q_command_name[] = "q";
const PROGMEM char ver_command_name[] = "ver";
const PROGMEM char version_command_name[] = "version";
const PROGMEM char clear_command_name[] = "clear";
const PROGMEM char cls_command_name[] = "cls";
const PROGMEM char pause_command_name[] = "pause";

const PROGMEM char eeprom_command_name[] = "eeprom";

const PROGMEM char disk_command_name[] = "disk";

const PROGMEM char read_command_name[] = "read";
const PROGMEM char cat_command_name[] = "cat";
const PROGMEM char hex_command_name[] = "hex";
const PROGMEM char hd_command_name[] = "hd";
const PROGMEM char write_command_name[] = "write";
const PROGMEM char append_command_name[] = "append";
const PROGMEM char add_command_name[] = "add";
const PROGMEM char list_command_name[] = "list";
const PROGMEM char ls_command_name[] = "ls";
const PROGMEM char dir_command_name[] = "dir";
const PROGMEM char rename_command_name[] = "rename";
const PROGMEM char mv_command_name[] = "mv";
const PROGMEM char delete_command_name[] = "delete";
const PROGMEM char del_command_name[] = "del";
const PROGMEM char rm_command_name[] = "rm";

const PROGMEM char stack_command_name[] = "stack";

const PROGMEM char heap_command_name[] = "heap";

const PROGMEM char run_command_name[] = "run";
const PROGMEM char start_command_name[] = "start";

const Command commands[] PROGMEM = {
    { random_command_name, &random_command }, { rand_command_name, &random_command },
    { sum_command_name, &sum_command },
    { average_command_name, &average_command },
    { hello_command_name, &hello_command },
    { help_command_name, &help_command },
    { exit_command_name, &exit_command }, { q_command_name, &exit_command },
    { version_command_name, &version_command }, { ver_command_name, &version_command },
    { clear_command_name, &clear_command }, { cls_command_name, &clear_command },
    { pause_command_name, &pause_command },

    { eeprom_command_name, &eeprom_command },

    { disk_command_name, &disk_command },

    { read_command_name, &read_command }, { cat_command_name, &read_command },
    { hex_command_name, &hex_command }, { hd_command_name, &hex_command },
    { write_command_name, &write_command },
    { append_command_name, &append_command }, { add_command_name, &append_command },
    { list_command_name, &list_command }, { ls_command_name, &list_command }, {dir_command_name, &list_command },
    { rename_command_name, &rename_command }, { mv_command_name, &rename_command },
    { delete_command_name, &delete_command }, { rm_command_name, &delete_command }, { del_command_name, &delete_command },

    { stack_command_name, &stack_command },

    { heap_command_name, &heap_command },

    { run_command_name, &run_command }, { start_command_name, &run_command }
};

// Util commands
void random_command(uint8_t argc, char **argv) {
    int16_t random_number;
    if (argc >= 3) {
        random_number = rand_int(strtol(argv[1], NULL, 10), strtol(argv[2], NULL, 10));
    } else if (argc >= 2) {
        random_number = rand_int(0, strtol(argv[1], NULL, 10));
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
            sum += strtol(argv[i], NULL, 10);
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
            sum += strtol(argv[i], NULL, 10);
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

// Disk command
void disk_command(uint8_t argc, char **argv) {
    if (argc >= 2) {
        if (!strcmp_P(argv[1], PSTR("alloc")) && argc >= 4) {
            uint16_t count = strtol(argv[2], NULL, 10);
            if (count == 0) count = 1;
            uint16_t address = disk_alloc(count);
            if (address != 0) {
                eeprom_write_byte(address, '\0');
                for (uint16_t i = 1; i < count; i++) {
                    eeprom_write_byte(address + i, argv[3][0]);
                }

                serial_print_P(PSTR("Address: "));
                serial_print_word(address, '0');
                serial_write('\n');
            } else {
                serial_println_P(PSTR("Not enough free disk space!"));
            }
        }

        if (!strcmp_P(argv[1], PSTR("free")) && argc >= 3) {
            uint16_t address = strtol(argv[2], NULL, 16);
            disk_free(address);
        }

        if (!strcmp_P(argv[1], PSTR("format"))) {
            disk_format();
        }

        if (!strcmp_P(argv[1], PSTR("dump"))) {
            eeprom_dump();
        }

        if (!strcmp_P(argv[1], PSTR("inspect")) || !strcmp_P(argv[1], PSTR("list"))) {
            disk_inspect();
        }
    } else {
        serial_println_P(PSTR("Help: disk alloc [count] [char], disk free [address], [count] [char], disk format, disk dump, disk inspect / list"));
    }
}

// File commands
void read_command(uint8_t argc, char **argv) {
    if (argc >= 2) {
        for (uint8_t i = 1; i < argc; i++) {
            int8_t file = file_open(argv[i], FILE_OPEN_MODE_READ);
            if (file != -1) {
                char buffer[16];
                uint16_t bytes_read;
                while ((bytes_read = file_read(file, (uint8_t *)buffer, sizeof(buffer) - 1)) != 0) {
                    buffer[bytes_read] = '\0';
                    serial_print(buffer);
                }
                file_close(file);
            } else {
                serial_println_P(PSTR("File open error!"));
            }
        }
    } else {
        serial_println_P(PSTR("Help: read [name]..."));
    }
}

void hex_command(uint8_t argc, char **argv) {
    if (argc >= 2) {
        for (uint8_t i = 1; i < argc; i++) {
            int8_t file = file_open(argv[i], FILE_OPEN_MODE_READ);
            if (file != -1) {
                serial_print_P(PSTR("     "));
                for (uint8_t x = 0; x < 16; x++) {
                    serial_print_byte(x, ' ');
                    serial_write(x == 15 ? '\t' : ' ');
                }
                for (uint8_t x = 0; x < 16; x++) {
                    serial_print_byte(x, '\0');
                    serial_write(x == 15 ? '\n' : ' ');
                }

                uint8_t size = file_size(file);
                uint8_t lines = size >> 4;
                if ((size & 15) != 0) lines++;
                if (lines == 0) lines = 1;
                for (uint16_t y = 0; y < lines; y++) {
                    serial_print_word(y << 4, '0');
                    serial_write(' ');

                    uint8_t buffer[16];
                    file_read(file, (uint8_t *)buffer, sizeof(buffer));

                    for (size_t x = 0; x < 16; x++) {
                        if (((y << 4) | x) < size) {
                            serial_print_byte(buffer[x], '0');
                        } else {
                            serial_print("  ");
                        }
                        serial_write(x == 15 ? '\t' : ' ');
                    }

                    for (size_t x = 0; x < 16; x++) {
                        char character;
                        if (((y << 4) | x) < size) {
                            character = buffer[x];
                            if (character < ' ' || character > '~') {
                                character = '.';
                            }
                        } else {
                            character = ' ';
                        }
                        serial_write(character);
                        serial_write(x == 15 ? '\n' : ' ');
                    }
                }

                file_close(file);
            } else {
                serial_println_P(PSTR("File open error!"));
            }
        }
    } else {
        serial_println_P(PSTR("Help: hex [name]..."));
    }
}

#define WRITE_BUFFER_SIZE 16

void write_command(uint8_t argc, char **argv) {
    if (argc >= 2) {
        int8_t file = file_open(argv[1], FILE_OPEN_MODE_WRITE);
        if (file != -1) {
            if (argc >= 3) {
                #ifndef ARDUINO
                    if (!strcmp(argv[2], "$")) {
                        FILE *in_file = fopen(argc >= 4 ? argv[3] : argv[1], "rb");
                        if (in_file != NULL) {
                            fseek(in_file, 0, SEEK_END);
                            size_t file_size = ftell(in_file);
                            fseek(in_file, 0, SEEK_SET);
                            uint8_t *file_buffer = malloc(file_size);
                            fread(file_buffer, 1, file_size, in_file);
                            file_write(file, file_buffer, file_size);
                            free(file_buffer);
                            fclose(in_file);
                        } else {
                            printf("File read error!\n");
                        }
                    } else {
                #endif

                for (uint8_t i = 2; i < argc; i++) {
                    if (file_write(file, (uint8_t *)argv[i], -1) == -1) {
                        serial_println_P(PSTR("File write error!"));
                    }

                    if (i != argc - 1) {
                        if (file_write(file, (uint8_t *)" ", 1) == -1) {
                            serial_println_P(PSTR("File write error!"));
                        }
                    }
                }

                #ifndef ARDUINO
                    }
                #endif
            } else {
                for (;;) {
                    char input_buffer[WRITE_BUFFER_SIZE * 2 + 1];
                    uint8_t input_buffer_size;
                    serial_read_line(input_buffer, &input_buffer_size, sizeof(input_buffer));
                    if (input_buffer[0] == '\0') {
                        break;
                    }

                    uint8_t write_buffer[WRITE_BUFFER_SIZE];
                    for (uint8_t i = 0; i < input_buffer_size; i++) {
                        if (input_buffer[i] >= '0' && input_buffer[i] <= '9') {
                            input_buffer[i] = input_buffer[i] - '0';
                        } else if (input_buffer[i] >= 'a' && input_buffer[i] <='f') {
                            input_buffer[i] = input_buffer[i] - 'a' + 10;
                        } else if (input_buffer[i] >= 'A' && input_buffer[i] <='F') {
                            input_buffer[i] = input_buffer[i] - 'A' + 10;
                        } else {
                            serial_println_P(PSTR("Not an hex character error!"));
                            break;
                        }

                        if (i % 2 == 0) {
                            write_buffer[i >> 1] = input_buffer[i] << 4;
                        } else {
                            write_buffer[i >> 1] = write_buffer[i >> 1] | input_buffer[i];
                        }
                    }

                    if (file_write(file, write_buffer, input_buffer_size >> 1) == -1) {
                        serial_println_P(PSTR("File write error!"));
                    }
                }
            }
            file_close(file);
        } else {
            serial_println_P(PSTR("File open error!"));
        }
    } else {
        serial_println_P(PSTR("Help: write [name] [text]..."));
    }
}

void append_command(uint8_t argc, char **argv) {
    if (argc >= 3) {
        int8_t file = file_open(argv[1], FILE_OPEN_MODE_APPEND);
        if (file != -1) {
            for (uint8_t i = 2; i < argc; i++) {
                if (file_write(file, (uint8_t *)argv[i], -1) == -1) {
                    serial_println_P(PSTR("File write error!"));
                }

                if (i != argc - 1) {
                    if (file_write(file, (uint8_t *)" ", 1) == -1) {
                        serial_println_P(PSTR("File write error!"));
                    }
                }
            }
            file_close(file);
        } else {
            serial_println_P(PSTR("File open error!"));
        }
    } else {
        serial_println_P(PSTR("Help: append [name] [text]..."));
    }
}

void list_command(uint8_t argc, char **argv) {
    (void)argc;
    (void)argv;
    serial_println_P(PSTR("Files:"));
    char file_name[32];
    uint16_t file_size;
    while (file_list(file_name, &file_size)) {
        serial_print_P(PSTR("- "));
        serial_print(file_name);
        serial_print_P(PSTR(": "));
        serial_print_number(file_size, '\0');
        serial_println_P(PSTR(" bytes"));
    }
}

void rename_command(uint8_t argc, char **argv) {
    if (argc >= 3) {
        for (uint8_t i = 1; i < argc; i += 2) {
            if (!file_rename(argv[i], argv[i + 1])) {
                serial_println_P(PSTR("File rename error!"));
            }
        }
    } else {
        serial_println_P(PSTR("Help: rename [[old_name] [new_name]]..."));
    }
}

void delete_command(uint8_t argc, char **argv) {
    if (argc >= 2) {
        for (uint8_t i = 1; i < argc; i++) {
            if (!file_delete(argv[i])) {
                serial_println_P(PSTR("File delete error!"));
            }
        }
    } else {
        serial_println_P(PSTR("Help: delete [name]..."));
    }
}

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
                char string_buffer[48];
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
            if (!strcmp_P(argv[2], PSTR("dword"))) {
                uint32_t *dword = heap_get_dword(id);
                if (dword != NULL) {
                    serial_print_P(PSTR("Dword: "));
                    serial_print_word(*dword >> 16, '0');
                    serial_print_word(*dword & 0xffff, '0');
                    serial_write('\n');
                }
            }
            if (!strcmp_P(argv[2], PSTR("float"))) {
                float *number = heap_get_float(id);
                if (number != NULL) {
                    serial_print_P(PSTR("Float: "));
                    char number_buffer[16];
                    #ifdef ARDUINO
                        dtostrf(*number, 0, 4, number_buffer);
                    #else
                        sprintf(number_buffer, "%.4f", *number);
                    #endif
                    serial_print(number_buffer);
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
            if (argc >= 3) {
                heap_clear(strtol(argv[2], NULL, 16));
            } else {
                heap_begin();
            }
        }

        if (!strcmp_P(argv[1], PSTR("dump"))) {
            serial_print_memory(heap, HEAP_SIZE);
        }

        if (!strcmp_P(argv[1], PSTR("inspect")) || !strcmp_P(argv[1], PSTR("list"))) {
            heap_inspect();
        }
    } else {
        serial_println_P(PSTR("Help: heap set [type] [id] [value], heap get [type] [id], heap clear [id]?, heap dump, heap inspect / list"));
    }
}

// Processor commands
void run_command(uint8_t argc, char **argv) {
    if (argc >= 2) {
        int8_t file = file_open(argv[1], FILE_OPEN_MODE_READ);
        if (file != -1) {
            Processor processor;
            processor_init(&processor, files[file].address + 1 + files[file].name_size + 2); // DIRTY

            for (size_t i = 0; processor.running; i++) {
                processor_clock(&processor);
                #ifdef DEBUG
                    if (i >= 1) {
                        #ifndef ARDUINO
                            serial_read_input();
                        #endif
                        while (serial_read() == '\0');
                    }
                #endif
            }

            file_close(file);
        } else {
            serial_println_P(PSTR("File open error!"));
        }
    } else {
        serial_println_P(PSTR("Help: run [name]"));
    }
}
