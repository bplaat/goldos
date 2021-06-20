#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdint.h>

typedef struct Command {
    const char *name;
    void (*command_function)(uint8_t argc, char **argv);
} Command;

#define COMMANDS_SIZE 17

extern const Command commands[];

// Util commands
void random_command(uint8_t argc, char **argv);

void sum_command(uint8_t argc, char **argv);

void average_command(uint8_t argc, char **argv);

void hello_command(uint8_t argc, char **argv);

void help_command(uint8_t argc, char **argv);

void exit_command(uint8_t argc, char **argv);

void version_command(uint8_t argc, char **argv);

void clear_command(uint8_t argc, char **argv);

void pause_command(uint8_t argc, char **argv);

// EEPROM command
void eeprom_command(uint8_t argc, char **argv);

// Disk command
void disk_command(uint8_t argc, char **argv);

// File commands
// void read_command(uint8_t argc, char **argv);

// void write_command(uint8_t argc, char **argv);

// void list_command(uint8_t argc, char **argv);

// Stack command
void stack_command(uint8_t argc, char **argv);

// Heap command
void heap_command(uint8_t argc, char **argv);

#endif
