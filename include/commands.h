#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdint.h>

typedef struct Command {
    const char *name;
    void (*command_function)(uint8_t argc, char **argv);
} Command;

extern Command commands[];

extern const uint8_t COMMANDS_SIZE;

void sum_command(uint8_t argc, char **argv);

void average_command(uint8_t argc, char **argv);

void hello_command(uint8_t argc, char **argv);

void help_command(uint8_t argc, char **argv);

void exit_command(uint8_t argc, char **argv);

void read_command(uint8_t argc, char **argv);

void write_command(uint8_t argc, char **argv);

#endif
