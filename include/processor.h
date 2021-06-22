#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <stdint.h>
#include <stdbool.h>

typedef struct Processor {
    bool running;
    bool debug;
    uint16_t pc;
    uint8_t r[32];
    uint16_t sp;
    union {
        struct {
            bool c : 1;
            bool z : 1;
            bool n : 1;
            bool v : 1;
            bool s : 1;
            bool h : 1;
            bool t : 1;
            bool i : 1;
        } flags;
        uint8_t data;
    } sreg;
    uint8_t ram[128];
    uint16_t pgm_address;
    uint32_t clock_ticks;
} Processor;

void processor_init(Processor *p, bool debug, uint16_t pgm_address);

uint8_t processor_read(Processor *p, uint16_t addr);

void processor_write(Processor *p, uint16_t addr, uint8_t data);

void processor_flags(Processor *p, uint8_t data, bool sub_zero_carry, bool overflow);

bool processor_add(Processor *p, uint8_t a, uint8_t b, bool carry_in, uint8_t *c);

void processor_add_with_carry(Processor *p, uint8_t a, uint8_t b, bool carry_in, uint8_t *c);

void processor_add_with_half_carry(Processor *p, uint8_t a, uint8_t b, bool carry_in, uint8_t *c);

bool processor_sub(Processor *p, uint8_t a, uint8_t b, bool carry_in, uint8_t *c, bool zero_carry);

void processor_sub_with_carry(Processor *p, uint8_t a, uint8_t b, bool carry_in, uint8_t *c, bool zero_carry);

void processor_sub_with_half_carry(Processor *p, uint8_t a, uint8_t b, bool carry_in, uint8_t *c, bool zero_carry);

void processor_clock(Processor *p);

#endif
