#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

// #define DEBUG

uint8_t *file_read(char *path) {
    FILE *file = fopen(path, "rb");
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    uint8_t *file_buffer = malloc(file_size);
    fread(file_buffer, 1, file_size, file);
    fclose(file);
    return file_buffer;
}

// A simple AtTiny25 Simulator / AVR2 ISA
#define STATUS_CARRY 0
#define STATUS_ZERO 1
#define STATUS_NEGATIVE 2
#define STATUS_OVERFLOW 3
#define STATUS_SIGN 4
#define STATUS_HALF_CARRY 5
#define STATUS_TEMPORARY 6
#define STATUS_INTERRUPT 7

#define bit(number, bit) ((number >> bit) & 1)
#define bit_set(number, bit) number |= (1 << bit);
#define bit_clear(number, bit) number &= ~(1 << bit);

typedef struct Processor {
    bool running;
    uint16_t instruction_pointer;
    uint8_t registers[32];
    uint16_t stack_pointer;
    uint8_t status_register;
    uint8_t ram[128];
    uint8_t *progmem;
} Processor;

void processor_init(Processor *processor, uint8_t *progmen) {
    processor->running = true;

    processor->instruction_pointer = 0;

    for (uint8_t i = 0; i < 32; i++) {
        processor->registers[i] = 0;
    }

    processor->stack_pointer = 0x60 + sizeof(processor->ram);
    processor->status_register = 0;

    for (uint8_t i = 0; i < 128; i++) {
        processor->ram[i] = 0;
    }

    processor->progmem = progmen;
}

uint8_t processor_memory_read(Processor *processor, uint16_t address) {
    if (address < 0x20) {
        return processor->registers[address];
    }
    if (address == 0x5d) {
        return processor->stack_pointer & 0xff;
    }
    if (address == 0x5e) {
        return (processor->stack_pointer >> 8) &0b11;
    }
    if (address == 0x5f) {
        return processor->status_register;
    }
    if (address >= 0x60) {
        // printf("= memory[0x%04x]\n", address);
        return processor->ram[address - 0x60];
    }
    return 0;
}

void processor_memory_write(Processor *processor, uint16_t address, uint8_t data) {
    if (address < 0x20) {
        processor->registers[address] = data;
    }
    if (address == 0x2f) {
        if (data == 0) {
            printf("ERROR PRINT NULL!\n");
            return;
        }
        #ifdef DEBUG
            printf("\n%c\n\n", (char)data);
        #else
            putchar((char)data);
        #endif
    }
    if (address == 0x5d) {
        processor->stack_pointer = (processor->stack_pointer & 0b1100000000) | data;
    }
    if (address == 0x5e) {
        processor->stack_pointer = ((data & 0b11) << 8) | (processor->stack_pointer & 0xff);
    }
    if (address == 0x5f) {
        processor->status_register = data;
    }
    if (address >= 0x60) {
        // printf("memory[0x%04x] = 0x%02x\n", address, data);
        processor->ram[address - 0x60] = data;
    }
}

void _processor_set_status_register_flags(Processor* processor, uint8_t number, bool overflow) {
    if (number == 0) {
        bit_set(processor->status_register, STATUS_ZERO);
    } else {
        bit_clear(processor->status_register, STATUS_ZERO);
    }

    if (bit(number, 7)) {
        bit_set(processor->status_register, STATUS_NEGATIVE);
    } else {
        bit_clear(processor->status_register, STATUS_NEGATIVE);
    }

    if (overflow) {
        bit_set(processor->status_register, STATUS_OVERFLOW);
    } else {
        bit_clear(processor->status_register, STATUS_OVERFLOW);
    }

    if (bit(processor->status_register, STATUS_NEGATIVE) != bit(processor->status_register, STATUS_OVERFLOW)) {
        bit_set(processor->status_register, STATUS_SIGN);
    } else {
        bit_clear(processor->status_register, STATUS_SIGN);
    }
}

void _processor_add(Processor* processor, uint8_t a, uint8_t b, bool with_carry, uint8_t *c, bool update_carry) {
    if (with_carry) {
        with_carry = bit(processor->status_register, STATUS_CARRY);
    } else {
        with_carry = false;
    }

    if ((int8_t)(a & 0b1111) + (int8_t)((b & 0b1111) + with_carry) > (int8_t)0b1111) {
        bit_set(processor->status_register, STATUS_HALF_CARRY);
    } else {
        bit_clear(processor->status_register, STATUS_HALF_CARRY);
    }

    if (__builtin_add_overflow((uint8_t)a, (uint8_t)(b + with_carry), (uint8_t *)c)) {
        _processor_set_status_register_flags(processor, *c, bit(processor->status_register, STATUS_CARRY) != true);
        if (update_carry) bit_set(processor->status_register, STATUS_CARRY);
    } else {
        _processor_set_status_register_flags(processor, *c, bit(processor->status_register, STATUS_CARRY) != false);
        if (update_carry) bit_clear(processor->status_register, STATUS_CARRY);
    }
}

void _processor_sub(Processor* processor, uint8_t a, uint8_t b, bool with_carry, uint8_t *c, bool update_carry) {
    if (with_carry) {
        with_carry = bit(processor->status_register, STATUS_CARRY);
    } else {
        with_carry = false;
    }

    if ((int8_t)(a & 0b1111) - (int8_t)((b & 0b1111) + with_carry) < (int8_t)0) {
        bit_set(processor->status_register, STATUS_HALF_CARRY);
    } else {
        bit_clear(processor->status_register, STATUS_HALF_CARRY);
    }

    if (__builtin_sub_overflow((uint8_t)a, (uint8_t)(b + with_carry), (uint8_t *)c)) {
        _processor_set_status_register_flags(processor, *c, bit(processor->status_register, STATUS_CARRY) != true);
        if (update_carry) bit_set(processor->status_register, STATUS_CARRY);
    } else {
        _processor_set_status_register_flags(processor, *c, bit(processor->status_register, STATUS_CARRY) != false);
        if (update_carry) bit_clear(processor->status_register, STATUS_CARRY);
    }
}

void processor_clock(Processor* processor) {
    if (!processor->running) return;

    uint16_t instruction = processor->progmem[processor->instruction_pointer] | (processor->progmem[processor->instruction_pointer + 1] << 8);
    processor->instruction_pointer += 2;

    // ###############################################################################
    // ###################### ARITHMETIC AND LOGIC INSTRUCTIONS ######################
    // ###############################################################################

    // Instruction: ADD Rd, Rr | ADC Rd, Rr
    // Encoding: 000c 11rd dddd rrrr
    if ((instruction & 0b1110110000000000) == 0b0000110000000000) {
        uint8_t Rd = (instruction >> 4) & 0b11111;
        uint8_t Rr = (bit(instruction, 8) << 4) | (instruction & 0b1111);
        bool with_carry = bit(instruction, 12);

        if (Rd == Rr) {
            // Instruction: LSL Rd
            // Enconding: 0000 11dd dddd dddd
            if (!with_carry) {
                #ifdef DEBUG
                    printf("Execute: lsl r%d\n", Rd);
                #endif
                if (bit(processor->registers[Rd], 7)) {
                    bit_set(processor->status_register, STATUS_CARRY);
                } else {
                    bit_clear(processor->status_register, STATUS_CARRY);
                }
                processor->registers[Rd] <<= 1;
                _processor_set_status_register_flags(processor, processor->registers[Rd], bit(processor->status_register, STATUS_NEGATIVE) != bit(processor->status_register, STATUS_CARRY));
                return;
            }

            // Instruction: ROL Rd
            // Enconding: 0001 11dd dddd dddd
            else {
                #ifdef DEBUG
                    printf("Execute: rol r%d\n", Rd);
                #endif
                bool old_carry = bit(processor->status_register, STATUS_CARRY);
                if (bit(processor->registers[Rd], 7)) {
                    bit_set(processor->status_register, STATUS_CARRY);
                } else {
                    bit_clear(processor->status_register, STATUS_CARRY);
                }
                processor->registers[Rd] <<= 1;
                processor->registers[Rd] |= old_carry;
                _processor_set_status_register_flags(processor, processor->registers[Rd], bit(processor->status_register, STATUS_NEGATIVE) != bit(processor->status_register, STATUS_CARRY));
                return;
            }
        }

        #ifdef DEBUG
            printf("Execute: %s r%d, r%d\n", with_carry ? "adc" : "add", Rd, Rr);
        #endif
        _processor_add(processor, processor->registers[Rd], processor->registers[Rr], with_carry, &processor->registers[Rd], true);
        return;
    }

    // Instruction: ADIW Rd+1:Rd, K
    // Encoding: 1001 0110 KKdd KKKK
    if ((instruction & 0b1111111100000000) == 0b1001011000000000) {
        uint8_t Rd = ((instruction >> 4) & 0b11) + 24;
        uint8_t K = ((instruction >> 2) & 0b110000) | (instruction & 0b1111);
        #ifdef DEBUG
            printf("Execute: adiw r%d, 0x%02x\n", Rd, K);
        #endif
        _processor_add(processor, processor->registers[Rd], K, false, &processor->registers[Rd], true);
        _processor_add(processor, processor->registers[Rd + 1], 0, true, &processor->registers[Rd + 1], true);
        return;
    }

    // Instruction: SUB Rd, Rr | SBC Rd, Rr
    // Encoding: 000c 10rd dddd rrrr
    if ((instruction & 0b1110110000000000) == 0b0000100000000000) {
        uint8_t Rd = (instruction >> 4) & 0b11111;
        uint8_t Rr = (bit(instruction, 8) << 4) | (instruction & 0b1111);
        bool with_carry = bit(instruction, 12) == 0;
        #ifdef DEBUG
            printf("Execute: %s r%d, r%d\n", with_carry ? "sbc" : "sub", Rd, Rr);
        #endif
        _processor_sub(processor, processor->registers[Rd], processor->registers[Rr], with_carry, &processor->registers[Rd], true);
        return;
    }

    // Instruction: SUBI Rd, K | SUBCI Rd, K
    // Encoding: 010c KKKK dddd KKKK
    if ((instruction & 0b1110000000000000) == 0b0100000000000000) {
        uint8_t Rd = ((instruction >> 4) & 0b1111) + 16;
        uint8_t K = ((instruction >> 4) & 0b11110000) | (instruction & 0b1111);
        bool with_carry = bit(instruction, 12) == 0;
        #ifdef DEBUG
            printf("Execute: %s r%d, 0x%02x\n", with_carry ? "sbci" : "subi", Rd, K);
        #endif
        _processor_sub(processor, processor->registers[Rd], K, with_carry, &processor->registers[Rd], true);
        return;
    }

    // Instruction: SBIW Rd+1:Rd, K
    // Encoding: 1001 0111 KKdd KKKK
    if ((instruction & 0b1111111100000000) == 0b1001011100000000) {
        uint8_t Rd = ((instruction >> 4) & 0b11) + 24;
        uint8_t K = ((instruction >> 2) & 0b110000) | (instruction & 0b1111);
        #ifdef DEBUG
            printf("Execute: sbiw r%d, 0x%02x\n", Rd, K);
        #endif
        _processor_sub(processor, processor->registers[Rd], K, false, &processor->registers[Rd], true);
        _processor_sub(processor, processor->registers[Rd + 1], 0, true, &processor->registers[Rd + 1], true);
        return;
    }

    // Instruction: AND Rd, Rr | TST Rd
    // Encoding: 0010 00rd dddd rrrr
    if ((instruction & 0b1111110000000000) == 0b0010000000000000) {
        uint8_t Rd = (instruction >> 4) & 0b11111;
        uint8_t Rr = (bit(instruction, 8) << 4) | (instruction & 0b1111);
        #ifdef DEBUG
            printf("Execute: and r%d, r%d\n", Rd, Rr);
        #endif
        processor->registers[Rd] &= processor->registers[Rr];
        _processor_set_status_register_flags(processor, processor->registers[Rd], false);
        return;
    }

    // Instruction: ANDI Rd, K | CBR Rd, K
    // Encoding: 0111 KKKK dddd KKKK
    if ((instruction & 0b1111000000000000) == 0b0111000000000000) {
        uint8_t Rd = ((instruction >> 4) & 0b1111) + 16;
        uint8_t K = ((instruction >> 4) & 0b11110000) | (instruction & 0b1111);
        #ifdef DEBUG
            printf("Execute: andi r%d, 0x%02x\n", Rd, K);
        #endif
        processor->registers[Rd] &= K;
        _processor_set_status_register_flags(processor, processor->registers[Rd], false);
        return;
    }

    // Instruction: OR Rd, Rr
    // Encoding: 0010 10rd dddd rrrr
    if ((instruction & 0b1111110000000000) == 0b0101000000000000) {
        uint8_t Rd = (instruction >> 4) & 0b11111;
        uint8_t Rr = (bit(instruction, 8) << 4) | (instruction & 0b1111);
        #ifdef DEBUG
            printf("Execute: or r%d, r%d\n", Rd, Rr);
        #endif
        processor->registers[Rd] |= processor->registers[Rr];
        _processor_set_status_register_flags(processor, processor->registers[Rd], false);
        return;
    }

    // Instruction: ORI Rd, K | SBR Rd, K
    // Encoding: 0110 KKKK dddd KKKK
    if ((instruction & 0b1111000000000000) == 0b0110000000000000) {
        uint8_t Rd = ((instruction >> 4) & 0b1111) + 16;
        uint8_t K = ((instruction >> 4) & 0b11110000) | (instruction & 0b1111);
        #ifdef DEBUG
            printf("Execute: ori r%d, 0x%02x\n", Rd, K);
        #endif
        processor->registers[Rd] |= K;
        _processor_set_status_register_flags(processor, processor->registers[Rd], false);
        return;
    }

    // Instruction: EOR Rd, Rr | CLR Rd
    // Encoding: 0010 01rd dddd rrrr
    if ((instruction & 0b1111110000000000) == 0b0010010000000000) {
        uint8_t Rd = (instruction >> 4) & 0b11111;
        uint8_t Rr = (bit(instruction, 9) << 4) | (instruction & 0b1111);
        #ifdef DEBUG
            printf("Execute: eor r%d, r%d\n", Rd, Rr);
        #endif
        processor->registers[Rd] ^= processor->registers[Rr];
        _processor_set_status_register_flags(processor, processor->registers[Rd], false);
        return;
    }

    // Instruction: COM Rd
    // Encoding: 1001 010d dddd 0000
    if ((instruction & 0b1111111000001111) == 0b1001010000000000) {
        uint8_t Rd = (instruction >> 4) & 0b11111;
        #ifdef DEBUG
            printf("Execute: com r%d\n", Rd);
        #endif
        _processor_sub(processor, 0xff, processor->registers[Rd], false, &processor->registers[Rd], true);
        return;
    }

    // Instruction: NEG Rd
    // Encoding: 1001 010d dddd 0001
    if ((instruction & 0b1111111000001111) == 0b1001010000000001) {
        uint8_t Rd = (instruction >> 4) & 0b11111;
        #ifdef DEBUG
            printf("Execute: neg r%d\n", Rd);
        #endif
        _processor_sub(processor, 0, processor->registers[Rd], false, &processor->registers[Rd], true);
        return;
    }

    // Instruction: INC Rd
    // Encoding: 1001 010d dddd 0011
    if ((instruction & 0b1111111000001111) == 0b1001010000000011) {
        uint8_t Rd = (instruction >> 4) & 0b11111;
        #ifdef DEBUG
            printf("Execute: inc r%d\n", Rd);
        #endif
        _processor_add(processor, processor->registers[Rd], 1, false, &processor->registers[Rd], false);
        return;
    }

    // Instruction: DEC Rd
    // Encoding: 1001 010d dddd 1010
    if ((instruction & 0b1111111000001111) == 0b1001010000001010) {
        uint8_t Rd = (instruction >> 4) & 0b11111;
        #ifdef DEBUG
            printf("Execute: dec r%d\n", Rd);
        #endif
        _processor_sub(processor, processor->registers[Rd], 1, false, &processor->registers[Rd], false);
        return;
    }

    // ###############################################################################
    // ############################# BRANCH INSTRUCTIONS #############################
    // ###############################################################################

    // Instruction: RJMP
    // Encoding: 1100 kkkk kkkk kkkk
    if ((instruction & 0b1111000000000000) == 0b1100000000000000) {
        int16_t k = (instruction & 0b111111111111) << 1;
        if (bit(k, 12)) k |= 0b1110000000000000;
        #ifdef DEBUG
            printf("Execute: rjmp%s%d\n", k >= 0 ? " +" : " ", k);
        #endif

        if (k == -2) {
            printf("STOP!\n");
            processor->running = false;
            return;
        }

        processor->instruction_pointer += k;
        return;
    }

    // Instruction: IJMP
    // Encoding: 1001 0100 0000 1001
    if (instruction == 0b1001010000001001) {
        #ifdef DEBUG
            printf("Execute: ijmp");
        #endif
        processor->instruction_pointer = processor->registers[30] | (processor->registers[31] << 8);
        return;
    }

    // Instruction: JMP
    // Encoding: 1001 010k kkkk 110k
    // Encoding: kkkk kkkk kkkk kkkk

    // Instruction: RCALL
    // Encoding: 1101 kkkk kkkk kkkk
    if ((instruction & 0b1111000000000000) == 0b1101000000000000) {
        int16_t k = (instruction & 0b111111111111) << 1;
        if (bit(k, 12)) k |= 0b1110000000000000;
        #ifdef DEBUG
            printf("Execute: rcall%s%d\n", k >= 0 ? " +" : " ", k);
        #endif
        uint16_t pc = processor->instruction_pointer;
        processor_memory_write(processor, processor->stack_pointer--, pc & 0xff);
        processor_memory_write(processor, processor->stack_pointer--, pc >> 8);
        processor->instruction_pointer += k;
        return;
    }

    // Instruction: ICALL
    // Encoding: 1001 0101 0000 1001
    if (instruction == 0b1001010100001001) {
        #ifdef DEBUG
            printf("Execute: icall");
        #endif
        uint16_t pc = processor->instruction_pointer + 1;
        processor_memory_write(processor, processor->stack_pointer--, pc & 0xff);
        processor_memory_write(processor, processor->stack_pointer--, pc >> 8);
        processor->instruction_pointer = processor->registers[30] | (processor->registers[31] << 8);
        return;
    }

    // Instruction: CALL
    // Encoding: 1001 010k kkkk 111k
    // Encoding: kkkk kkkk kkkk kkkk

    // Instruction: RET | RETI
    // Encoding: 1001 0101 000i 1000
    if ((instruction & 0b1111111111101111) == 0b1001010100001000) {
        bool with_interrupt = bit(instruction, 4);
        #ifdef DEBUG
            printf("Execute: ret%s\n", with_interrupt ? "i" : "");
        #endif
        processor->instruction_pointer = (processor_memory_read(processor, ++processor->stack_pointer) << 8) | processor_memory_read(processor, ++processor->stack_pointer);
        if (with_interrupt) {
            bit_set(processor->status_register, STATUS_INTERRUPT);
        }
        return;
    }

    // Instruction: CPSE
    // Encoding: 0001 00rd dddd rrrr
    if ((instruction & 0b1111110000000000) == 0b0001000000000000) {
        uint8_t Rd = (instruction >> 4) & 0b11111;
        uint8_t Rr = (bit(instruction, 9) << 4) | (instruction & 0b1111);
        #ifdef DEBUG
            printf("Execute: cpse r%d, r%d\n", Rd, Rr);
        #endif
        if (processor->registers[Rd] == processor->registers[Rr]) {
            processor->instruction_pointer += 2;
        }
        return;
    }

    // Instruction: CP | CPC
    // Encoding: 000c 01rd dddd rrrr
    if ((instruction & 0b1110110000000000) == 0b0000010000000000) {
        uint8_t Rd = (instruction >> 4) & 0b11111;
        uint8_t Rr = (bit(instruction, 8) << 4) | (instruction & 0b1111);
        bool with_carry = bit(instruction, 12) == 0;
        #ifdef DEBUG
            printf("Execute: %s r%d, r%d\n", with_carry ? "cpc" : "cp", Rd, Rr);
        #endif
        uint8_t c;
        _processor_sub(processor, processor->registers[Rd], processor->registers[Rr], with_carry, &c, true);
        return;
    }

    // Instruction: CPI
    // Encoding: 0011 KKKK dddd KKKK
    if ((instruction & 0b1111000000000000) == 0b0011000000000000) {
        uint8_t Rd = ((instruction >> 4) & 0b1111) + 16;
        uint8_t K = ((instruction >> 4) & 0b11110000) | (instruction & 0b1111);
        #ifdef DEBUG
            printf("Execute: cpi r%d, 0x%02x\n", Rd, K);
        #endif
        uint8_t c;
        _processor_sub(processor, processor->registers[Rd], K, false, &c, true);
        return;
    }

    // Instruction: SBRC | SBRS
    // Encoding: 1111 11sr rrrr 0bbb
    if ((instruction & 0b1111110000001000) == 0b1111110000000000) {
        uint8_t Rr = (instruction >> 4) & 0b11111;
        uint8_t b = instruction & 0b111;
        bool is_set = bit(instruction, 9) == 0;
        #ifdef DEBUG
            printf("Execute: SBRC...");
        #endif
        if (bit(processor->registers[Rr], b) == is_set) {
            processor->instruction_pointer += 2;
        }
        return;
    }

    // Instruction: SBIC | SBIS
    // Encoding: 1001 10s1 AAAA Abbb
    if ((instruction & 0b1111110100000000) == 0b1001100100000000) {
        uint8_t A = (instruction >> 3) & 0b11111;
        uint8_t b = instruction & 0b111;
        bool is_set = bit(instruction, 9) == 0;
        #ifdef DEBUG
            printf("Execute: SBIC...");
        #endif
        if (bit(processor_memory_read(processor, A + 0x20), b) == is_set) {
            processor->instruction_pointer += 2;
        }
        return;
    }

    // Instruction: BRBS | BRBC | ...
    // Encoding: 1111 0skk kkkk ksss
    if ((instruction & 0b1111100000000000) == 0b1111000000000000) {
        int16_t k = (instruction >> 2) & 0b11111110;
        if (bit(k, 7)) k |= 0b1111111110000000;
        uint8_t s = instruction & 0b111;
        bool is_set = bit(instruction, 9) == 0;
        #ifdef DEBUG
            printf("Execute: %s %d,%s%d\n", is_set ? "brbs" : "brbc", s, k > 0 ? " +" : " ", k);
        #endif
        if (bit(processor->status_register, s) == is_set) {
            processor->instruction_pointer += k;
        }
        return;
    }

    // ###############################################################################
    // ######################### DATA TRANSFER INSTRUCTIONS ##########################
    // ###############################################################################

    // Instruction: MOV
    // Encoding: 0010 11rd dddd rrrr
    if ((instruction & 0b1111110000000000) == 0b0010110000000000) {
        uint8_t Rd = (instruction >> 4) & 0b11111;
        uint8_t Rr = (bit(instruction, 8) << 4) | (instruction & 0b1111);
        #ifdef DEBUG
            printf("Execute: mov r%d, r%d\n", Rd, Rr);
        #endif
        processor->registers[Rd] = processor->registers[Rr];
        return;
    }

    // Instruction: MOVW
    // Encoding: 0000 0001 dddd rrrr
    if ((instruction & 0b1111111100000000) == 0b0000000100000000) {
        uint8_t Rd = ((instruction >> 4) & 0b1111) << 1;
        uint8_t Rr = (instruction & 0b1111) << 1;
        processor->registers[Rd] = processor->registers[Rr];
        processor->registers[Rd + 1] = processor->registers[Rr + 1];
        #ifdef DEBUG
            printf("Execute: movw r%d, r%d\n", Rd, Rr);
            printf("registers[%d] = %02x\n", Rd, processor->registers[Rd]);
            printf("registers[%d] = %02x\n", Rd + 1, processor->registers[Rd + 1]);
        #endif
        return;
    }

    // Instruction: LDI
    // Encoding: 1110 KKKK dddd KKKK
    if ((instruction & 0b1111000000000000) == 0b1110000000000000) {
        uint8_t Rd = ((instruction >> 4) & 0b1111) + 16;
        uint8_t K = (((instruction >> 8) & 0b1111) << 4) | (instruction & 0b1111);
        processor->registers[Rd] = K;
        #ifdef DEBUG
            printf("Execute: ldi r%d, 0x%02x\n", Rd, K);
            printf("registers[%d] = %02x\n", Rd, processor->registers[Rd]);
        #endif
        return;
    }

    // Instruction: LD Rd, X
    // Encoding: 1001 000d dddd 1100
    if ((instruction & 0b1111111000001111) == 0b1001000000001100) {
        uint8_t Rd = (instruction >> 4) & 0b11111;
        #ifdef DEBUG
            printf("Execute: ld Rd, X\n");
        #endif
        uint16_t X = processor->registers[26] | (processor->registers[27] << 8);
        processor->registers[Rd] = processor_memory_read(processor, X);
        return;
    }

    // Instruction: LD Rd, X+
    // Encoding: 1001 000d dddd 1101
    if ((instruction & 0b1111111000001111) == 0b1001000000001101) {
        uint8_t Rd = (instruction >> 4) & 0b11111;
        #ifdef DEBUG
            printf("Execute: ld Rd, X+\n");
        #endif
        uint16_t X = processor->registers[26] | (processor->registers[27] << 8);
        processor->registers[Rd] = processor_memory_read(processor, X);
        X++;
        processor->registers[26] = X & 0xff;
        processor->registers[27] = X >> 8;
        return;
    }

    // Instruction: LD Rd, -X
    // Encoding: 1001 000d dddd 1110
    if ((instruction & 0b1111111000001111) == 0b1001000000001110) {
        uint8_t Rd = (instruction >> 4) & 0b11111;
        #ifdef DEBUG
            printf("Execute: ld Rd, -X\n");
        #endif
        uint16_t X = processor->registers[26] | (processor->registers[27] << 8);
        X--;
        processor->registers[Rd] = processor_memory_read(processor, X);
        processor->registers[26] = X & 0xff;
        processor->registers[27] = X >> 8;
        return;
    }

    // Instruction: LD Rd, Y
    // Encoding: 1000 000d dddd 1000
    if ((instruction & 0b1111111000001111) == 0b1000000000001000) {
        uint8_t Rd = (instruction >> 4) & 0b11111;
        #ifdef DEBUG
            printf("Execute: ld Rd, Y\n");
        #endif
        uint16_t Y = processor->registers[28] | (processor->registers[29] << 8);
        processor->registers[Rd] = processor_memory_read(processor, Y);
        return;
    }

    // Instruction: LD Rd, Y+
    // Encoding: 1001 000d dddd 1001
    if ((instruction & 0b1111111000001111) == 0b1001000000001001) {
        uint8_t Rd = (instruction >> 4) & 0b11111;
        #ifdef DEBUG
            printf("Execute: ld Rd, Y+\n");
        #endif
        uint16_t Y = processor->registers[28] | (processor->registers[29] << 8);
        processor->registers[Rd] = processor_memory_read(processor, Y);
        Y++;
        processor->registers[26] = Y & 0xff;
        processor->registers[27] = Y >> 8;
        return;
    }


    // Instruction: LD Rd, -Y
    // Encoding: 1001 000d dddd 1010
    if ((instruction & 0b1111111000001111) == 0b1001000000001010) {
        uint8_t Rd = (instruction >> 4) & 0b11111;
        #ifdef DEBUG
            printf("Execute: ld Rd, -Y\n");
        #endif
        uint16_t Y = processor->registers[28] | (processor->registers[29] << 8);
        Y--;
        processor->registers[Rd] = processor_memory_read(processor, Y);
        processor->registers[26] = Y & 0xff;
        processor->registers[27] = Y >> 8;
        return;
    }

    // // Instruction: LDD Rd, Y + q
    // // Encoding: 10q0 qq0d dddd 1qqq
    // if ((instruction & 0b1101001000001000) == 0b1000000000001000) {
    //     uint8_t Rd = (instruction >> 4) & 0b11111;
    //     uint8_t q = (bit(instruction, 13) << 5) | (bit(instruction, 11) << 4) | (bit(instruction, 10) << 3) | (instruction & 0b111);
    //     #ifdef DEBUG
    //         printf("Execute: ldD Rd, Y + q");
    //     #endif
    //     uint16_t Y = processor->registers[28] | (processor->registers[29] << 8);
    //     processor->registers[Rd] = processor_memory_read(processor, Y + q);
    //     return;
    // }

    // Instruction: LD Rd, Z
    // Encoding: 1000 000d dddd 0000
    if ((instruction & 0b1111111000001111) == 0b1000000000000000) {
        uint8_t Rd = (instruction >> 4) & 0b11111;
        #ifdef DEBUG
            printf("Execute: ld Rd, Z\n");
        #endif
        uint16_t Z = processor->registers[30] | (processor->registers[31] << 8);
        processor->registers[Rd] = processor_memory_read(processor, Z);
        return;
    }

    // Instruction: LD Rd, Z+
    // Encoding: 1001 000d dddd 0001
    if ((instruction & 0b1111111000001111) == 0b1001000000000001) {
        uint8_t Rd = (instruction >> 4) & 0b11111;
        #ifdef DEBUG
            printf("Execute: ld Rd, Z+\n");
        #endif
        uint16_t Z = processor->registers[30] | (processor->registers[31] << 8);
        processor->registers[Rd] = processor_memory_read(processor, Z);
        Z++;
        processor->registers[30] = Z & 0xff;
        processor->registers[31] = Z >> 8;
        return;
    }

    // Instruction: LD Rd, -Z
    // Encoding: 1001 000d dddd 0010
    if ((instruction & 0b1111111000001111) == 0b1001000000000010) {
        uint8_t Rd = (instruction >> 4) & 0b11111;
        #ifdef DEBUG
            printf("Execute: ld Rd, -Z\n");
        #endif
        uint16_t Z = processor->registers[30] | (processor->registers[31] << 8);
        Z--;
        processor->registers[Rd] = processor_memory_read(processor, Z);
        processor->registers[30] = Z & 0xff;
        processor->registers[31] = Z >> 8;
        return;
    }

    // // Instruction: LDD Rd, Z + q
    // // Encoding: 10q0 qq0d dddd 0qqq
    // if ((instruction & 0b1101001000001000) == 0b1000000000000000) {
    //     uint8_t Rd = (instruction >> 4) & 0b11111;
    //     uint8_t q = (bit(instruction, 13) << 5) | (bit(instruction, 11) << 4) | (bit(instruction, 10) << 3) | (instruction & 0b111);
    //     #ifdef DEBUG
    //         printf("Execute: ldD Rd, Z + q");
    //     #endif
    //     uint16_t Z = processor->registers[30] | (processor->registers[31] << 8);
    //     processor->registers[Rd] = processor_memory_read(processor, Z + q);
    //     return;
    // }

    // Instruction: LDS Rd, k
    // Encoding: 1001 000d dddd 0000
    // Encoding: kkkk kkkk kkkk kkkk

    // Instruction: ST X, Rr
    // Encoding: 1001 001r rrrr 1100
    if ((instruction & 0b1111111000001111) == 0b1001001000001100) {
        uint8_t Rr = (instruction >> 4) & 0b11111;
        #ifdef DEBUG
            printf("Execute: st X, r%d\n", Rr);
        #endif
        uint16_t X = processor->registers[26] | (processor->registers[27] << 8);
        processor_memory_write(processor, X, processor->registers[Rr]);
        return;
    }

    // Instruction: ST X+, Rr
    // Encoding: 1001 001r rrrr 1101
    if ((instruction & 0b1111111000001111) == 0b1001001000001101) {
        uint8_t Rr = (instruction >> 4) & 0b11111;
        #ifdef DEBUG
            printf("Execute: st X+, r%d\n", Rr);
        #endif
        uint16_t X = processor->registers[26] | (processor->registers[27] << 8);
        processor_memory_write(processor, X, processor->registers[Rr]);
        X++;
        processor->registers[26] = X & 0xff;
        processor->registers[27] = X >> 8;
        return;
    }

    // Instruction: ST -X, Rr
    // Encoding: 1001 001r rrrr 1110
    if ((instruction & 0b1111111000001111) == 0b1001001000001110) {
        uint8_t Rr = (instruction >> 4) & 0b11111;
        #ifdef DEBUG
            printf("Execute: st -X, r%d\n", Rr);
        #endif
        uint16_t X = processor->registers[26] | (processor->registers[27] << 8);
        X--;
        processor_memory_write(processor, X, processor->registers[Rr]);
        processor->registers[26] = X & 0xff;
        processor->registers[27] = X >> 8;
        return;
    }

    // Instruction: ST Y, Rr
    // Encoding: 1000 001r rrrr 1000
    if ((instruction & 0b1111111000001111) == 0b1000001000001000) {
        uint8_t Rr = (instruction >> 4) & 0b11111;
        #ifdef DEBUG
            printf("Execute: st Y, r%d\n", Rr);
        #endif
        uint16_t Y = processor->registers[28] | (processor->registers[29] << 8);
        processor_memory_write(processor, Y, processor->registers[Rr]);
        return;
    }

    // Instruction: ST Y+, Rr
    // Encoding: 1001 001r rrrr 1001
    if ((instruction & 0b1111111000001111) == 0b1001001000001001) {
        uint8_t Rr = (instruction >> 4) & 0b11111;
        #ifdef DEBUG
            printf("Execute: st Y+, r%d\n", Rr);
        #endif
        uint16_t Y = processor->registers[28] | (processor->registers[29] << 8);
        processor_memory_write(processor, Y, processor->registers[Rr]);
        Y++;
        processor->registers[28] = Y & 0xff;
        processor->registers[29] = Y >> 8;
        return;
    }

    // Instruction: ST -Y, Rr
    // Encoding: 1001 001r rrrr 1010
    if ((instruction & 0b1111111000001111) == 0b1001001000001010) {
        uint8_t Rr = (instruction >> 4) & 0b11111;
        #ifdef DEBUG
            printf("Execute: st -Y, r%d\n", Rr);
        #endif
        uint16_t Y = processor->registers[28] | (processor->registers[29] << 8);
        Y--;
        processor_memory_write(processor, Y, processor->registers[Rr]);
        processor->registers[28] = Y & 0xff;
        processor->registers[29] = Y >> 8;
        return;
    }

    // // Instruction: STD Y + q, Rr
    // // Encoding: 10q0 qq1r rrrr 1qqq
    // if ((instruction & 0b1101001000001000) == 0b1000001000001000) {
    //     uint8_t Rr = (instruction >> 4) & 0b11111;
    //     uint8_t q = (bit(instruction, 13) << 5) | (bit(instruction, 11) << 4) | (bit(instruction, 10) << 3) | (instruction & 0b111);
    //     #ifdef DEBUG
    //         printf("Execute: stD Y + 0x%02x, r%d\n", q, Rr);
    //     #endif
    //     uint16_t Y = processor->registers[28] | (processor->registers[29] << 8);
    //     processor_memory_write(processor, Y + q, processor->registers[Rr]);
    //     return;
    // }

    // Instruction: ST Z, Rr
    // Encoding: 1000 001r rrrr 0000
    if ((instruction & 0b1111111000001111) == 0b1000001000000000) {
        uint8_t Rr = (instruction >> 4) & 0b11111;
        #ifdef DEBUG
            printf("Execute: st Z, r%d\n", Rr);
        #endif
        uint16_t Z = processor->registers[30] | (processor->registers[31] << 8);
        processor_memory_write(processor, Z, processor->registers[Rr]);
        return;
    }

    // Instruction: ST Z+, Rr
    // Encoding: 1001 001r rrrr 0001
    if ((instruction & 0b1111111000001111) == 0b1001001000000001) {
        uint8_t Rr = (instruction >> 4) & 0b11111;
        #ifdef DEBUG
            printf("Execute: st Z+, r%d\n", Rr);
        #endif
        uint16_t Z = processor->registers[30] | (processor->registers[31] << 8);
        processor_memory_write(processor, Z, processor->registers[Rr]);
        Z++;
        processor->registers[30] = Z & 0xff;
        processor->registers[31] = Z >> 8;
        return;
    }

    // Instruction: ST -Z, Rr
    // Encoding: 1001 001r rrrr 0010
    if ((instruction & 0b1111111000001111) == 0b1001001000000010) {
        uint8_t Rr = (instruction >> 4) & 0b11111;
        #ifdef DEBUG
            printf("Execute: st -Z, r%d\n", Rr);
        #endif
        uint16_t Z = processor->registers[30] | (processor->registers[31] << 8);
        Z--;
        processor_memory_write(processor, Z, processor->registers[Rr]);
        processor->registers[30] = Z & 0xff;
        processor->registers[31] = Z >> 8;
        return;
    }

    // // Instruction: STD Z + q, Rr
    // // Encoding: 10q0 qq1r rrrr 0qqq
    // if ((instruction & 0b1101001000001000) == 0b1000001000000000) {
    //     uint8_t Rr = (instruction >> 4) & 0b11111;
    //     uint8_t q = (bit(instruction, 13) << 5) | (bit(instruction, 11) << 4) | (bit(instruction, 10) << 3) | (instruction & 0b111);
    //     #ifdef DEBUG
    //         printf("Execute: stD Z + 0x%02x, r%d\n", q, Rr);
    //     #endif
    //     uint16_t Z = processor->registers[28] | (processor->registers[29] << 8);
    //     processor_memory_write(processor, Z + q, processor->registers[Rr]);
    //     return;
    // }

    // Instruction: STS k, Rr
    // Encoding: 1001 001d dddd 0000
    // Encoding: kkkk kkkk kkkk kkkk

    // Instruction: LPM
    // Encoding: 1001 0101 1100 1000
    if (instruction == 0b1001010111001000) {
        uint16_t Z = processor->registers[30] | (processor->registers[31] << 8);
        #ifdef DEBUG
            printf("Execute: lpm");
        #endif
        processor->registers[0] = processor->progmem[Z];
        return;
    }

    // Instruction: LPM Rd, Z | LPM Rd, Z+
    // Enconding: 1001 000d dddd 01mm
    if ((instruction & 0b1111111000001100) == 0b1001000000000100) {
        uint8_t Rd = (instruction >> 4) & 0b11111;
        uint8_t mode = instruction & 0b11;
        uint16_t Z = processor->registers[30] | (processor->registers[31] << 8);
        #ifdef DEBUG
            if (mode == 0b00) printf("Execute: lpm r%d, Z\n", Rd);
            if (mode == 0b01) printf("Execute: lpm r%d, Z+\n", Rd);
            printf("progmem[0x%04x] = %02x\n", Z, processor->progmem[Z]);
        #endif
        processor->registers[Rd] = processor->progmem[Z];
        if (mode == 0b01) {
            Z++;
            processor->registers[30] = Z & 0xff;
            processor->registers[31] = Z >> 8;
        }
        return;
    }

    // Instruction: IN Rd, A
    // Encoding: 1011 0AAr rrrr AAAA
    if ((instruction & 0b1111100000000000) == 0b1011000000000000) {
        uint8_t Rd = (instruction >> 4) & 0b11111;
        uint8_t A = (((instruction >> 9) & 0b11) << 4) | (instruction & 0b1111);
        #ifdef DEBUG
            printf("Execute: in r%d, 0x%02x\n", Rd, A);
        #endif
        processor->registers[Rd] = processor_memory_read(processor, A + 0x20);
        return;
    }

    // Instruction: OUT A, Rr
    // Encoding: 1011 1AAr rrrr AAAA
    if ((instruction & 0b1111100000000000) == 0b1011100000000000) {
        uint8_t Rr = (instruction >> 4) & 0b11111;
        uint8_t A = (((instruction >> 9) & 0b11) << 4) | (instruction & 0b1111);
        #ifdef DEBUG
            printf("Execute: out 0x%02x, r%d\n", A, Rr);
        #endif
        processor_memory_write(processor, A + 0x20, processor->registers[Rr]);
        return;
    }

    // Instruction: PUSH Rd
    // Encoding: 1001 001d dddd 1111
    if ((instruction & 0b1111111000001111) == 0b1001001000001111) {
        uint8_t Rd = (instruction >> 4) & 0b11111;
        #ifdef DEBUG
            printf("Execute: push r%d\n", Rd);
        #endif
        processor_memory_write(processor, processor->stack_pointer--, processor->registers[Rd]);
        return;
    }

    // Instruction: POP Rd
    // Encoding: 1001 000d dddd 1111
    if ((instruction & 0b1111111000001111) == 0b1001000000001111) {
        uint8_t Rd = (instruction >> 4) & 0b11111;
        #ifdef DEBUG
            printf("Execute: pop r%d\n", Rd);
        #endif
        processor->registers[Rd] = processor_memory_read(processor, ++processor->stack_pointer);
        return;
    }

    // ###############################################################################
    // ####################### BIT AND BIT-TEST INSTRUCTIONS #########################
    // ###############################################################################

    // Instruction: SBI A, b | CBI A, b
    // Encoding: 1001 10s0 AAAA Abbb
    if ((instruction & 0b1111110100000000) == 0b1001100000000000) {
        uint8_t A = (instruction >> 3) & 0b11111;
        uint8_t b = instruction & 0b111;
        uint8_t set = bit(instruction, 9);
        #ifdef DEBUG
            printf("Execute: %s 0x%02x, %d\n", set ? "sbi" : "cbi", A, b);
        #endif
        uint8_t data = processor_memory_read(processor, A + 0x20);
        if (set) {
            bit_set(data, b);
        } else {
            bit_clear(data, b);
        }
        processor_memory_write(processor, A + 0x20, data);
        return;
    }

    // Instruction: LSR Rd
    // Enconding: 1001 010d dddd 0110
    if ((instruction & 0b1111111000001111) == 0b1001010000000110) {
        uint8_t Rd = (instruction >> 4) & 0b11111;
        #ifdef DEBUG
            printf("Execute: lsr r%d\n", Rd);
        #endif
        if (bit(processor->registers[Rd], 0)) {
            bit_set(processor->status_register, STATUS_CARRY);
        } else {
            bit_clear(processor->status_register, STATUS_CARRY);
        }
        processor->registers[Rd] >>= 1;
        _processor_set_status_register_flags(processor, processor->registers[Rd], bit(processor->status_register, STATUS_NEGATIVE) != bit(processor->status_register, STATUS_CARRY));
        return;
    }

    // Instruction: ROR Rd
    // Enconding: 1001 010d dddd 0111
    if ((instruction & 0b1111111000001111) == 0b1001010000000111) {
        uint8_t Rd = (instruction >> 4) & 0b11111;
        #ifdef DEBUG
            printf("Execute: ror r%d\n", Rd);
        #endif
        bool old_carry = bit(processor->status_register, STATUS_CARRY);
        if (bit(processor->registers[Rd], 0)) {
            bit_set(processor->status_register, STATUS_CARRY);
        } else {
            bit_clear(processor->status_register, STATUS_CARRY);
        }
        processor->registers[Rd] >>= 1;
        processor->registers[Rd] |= old_carry << 7;
        _processor_set_status_register_flags(processor, processor->registers[Rd], bit(processor->status_register, STATUS_NEGATIVE) != bit(processor->status_register, STATUS_CARRY));
        return;
    }

    // Instruction: ASR Rd
    // Encoding: 1001 010d dddd 0101
    if ((instruction & 0b1111111000001111) == 0b1001010000000101) {
        uint8_t Rd = (instruction >> 4) & 0b11111;
        #ifdef DEBUG
            printf("Execute: asr r%d\n", Rd);
        #endif
        bool old_top_bit = bit(processor->registers[Rd], 7);
        if (bit(processor->registers[Rd], 0)) {
            bit_set(processor->status_register, STATUS_CARRY);
        } else {
            bit_clear(processor->status_register, STATUS_CARRY);
        }
        processor->registers[Rd] >>= 1;
        if (old_top_bit) {
            processor->registers[Rd] |= 1 << 7;
        }
        _processor_set_status_register_flags(processor, processor->registers[Rd], bit(processor->status_register, STATUS_NEGATIVE) != bit(processor->status_register, STATUS_CARRY));
        return;
    }

    // Instruction: SWAP Rd
    // Encoding: 1001 010d dddd 0010
    if ((instruction & 0b1111111000001111) == 0b1001010000000010) {
        uint8_t Rd = (instruction >> 4) & 0b11111;
        uint8_t firstNibble = processor->registers[Rd] & 0b1111;
        uint8_t secondNibble = processor->registers[Rd] >> 4;
        #ifdef DEBUG
            printf("Execute: swap r%d\n", Rd);
        #endif
        processor->registers[Rd] = (firstNibble << 4) | secondNibble;
        return;
    }

    // Instruction: BSET s | SEC | SEN | SEZ | SEI | SES | SEV | SET | SEH
    // Instruction: BCLR s | CLC | CLN | CLZ | CLI | CLS | CLV | CLT | CLH
    // Encoding: 1001 0100 csss 1000
    if ((instruction & 0b1111111100001111) == 0b1001010000001000) {
        uint8_t s = (instruction >> 4) & 0b111;
        uint8_t clear = bit(instruction, 7) == 0;
        #ifdef DEBUG
            printf("Execute: %s %d\n", clear ? "bclr" : "bset", s);
        #endif
        if (clear) {
            bit_clear(processor->status_register, s);
        } else {
            bit_set(processor->status_register, s);
        }
        return;
    }

    // Instruction: BST Rd, b
    // Encoding: 1111 101d dddd 0bbb
    if ((instruction & 0b1111111000001000) == 0b1111101000000000) {
        uint8_t Rd = (instruction >> 4) & 0b11111;
        uint8_t b = instruction & 0b111;
        #ifdef DEBUG
            printf("Execute: bst r%d, %d\n", Rd, b);
        #endif
        if (bit(processor->registers[Rd], b)) {
            bit_set(processor->status_register, STATUS_TEMPORARY);
        } else {
            bit_clear(processor->status_register, STATUS_TEMPORARY);
        }
        return;
    }

    // Instruction: BLD Rd, b
    // Encoding: 1111 100d dddd 0bbb
    if ((instruction & 0b1111111000001000) == 0b1111100000000000) {
        uint8_t Rd = (instruction >> 4) & 0b11111;
        uint8_t b = instruction & 0b111;
        #ifdef DEBUG
            printf("Execute: bld r%d, %d\n", Rd, b);
        #endif
        if (bit(processor->status_register, STATUS_TEMPORARY)) {
            bit_set(processor->registers[Rd], b);
        } else {
            bit_clear(processor->registers[Rd], b);
        }
        return;
    }

    // Instruction: NOP
    // Encoding: 0000 0000 0000 0000
    // Instruction: SLEEP
    // Encoding: 1001 0101 1000 1000
    // Instruction: WDR
    // Encoding: 1001 0101 1010 1000
    // Instruction: BREAK
    // Encoding: 1001 0101 1001 1000
    // Instruction: SPM
    // Encoding: 1001 0101 1110 1000
    if (instruction == 0 || instruction == 0b1001010110001000 || instruction == 0b1001010110101000 || instruction == 0b1001010110011000|| instruction == 0b1001010111101000) {
        #ifdef DEBUG
            printf("Execute: nop");
        #endif
        return;
    }

    printf("Unexpected instruction: %02x %02x\n", instruction & 0xff, instruction >> 8);
    exit(1);
}

int main(void) {
    uint8_t *hello_program = file_read("examples/hello.prg");

    Processor processor;
    processor_init(&processor, hello_program);

    for (uint16_t i = 0; i < 1000; i++) {
        processor_clock(&processor);
    }

    free(hello_program);
}
