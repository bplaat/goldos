#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdint.h>

typedef struct Command {
    const char *name;
    void (*command_function)(uint8_t argc, char **argv);
} Command;

#define COMMANDS_SIZE 22

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

// Disk commands
void dump_command(uint8_t argc, char **argv);

void format_command(uint8_t argc, char **argv);

void inspect_command(uint8_t argc, char **argv);

void alloc_command(uint8_t argc, char **argv);

void free_command(uint8_t argc, char **argv);

void realloc_command(uint8_t argc, char **argv);

void test_command(uint8_t argc, char **argv);

// File commands
// void read_command(uint8_t argc, char **argv);

// void write_command(uint8_t argc, char **argv);

// void list_command(uint8_t argc, char **argv);

// Stack commands
void stack_command(uint8_t argc, char **argv);

#endif
