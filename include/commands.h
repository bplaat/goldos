#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdint.h>

void sum_command(uint8_t argc, uint8_t** argv);

void average_command(uint8_t argc, uint8_t** argv);

void hello_command(uint8_t argc, uint8_t** argv);

void help_command(uint8_t argc, uint8_t** argv);

void format_command(uint8_t argc, uint8_t** argv);

typedef struct Command {
    uint8_t* name;
    void (*command_function)(uint8_t argc, uint8_t** argv);
} Command;

extern Command commands[];
extern const uint8_t COMMANDS_SIZE;

#endif
