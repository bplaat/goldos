#include "processor.h"
#include <stdio.h>
#include "utils.h"
#include "serial.h"
#include "eeprom.h"
#include "file.h"

void processor_init(Processor *p, bool debug, uint16_t pgm_address) {
    p->running = true;
    p->debug = debug;
    p->pc = 0;
    for (uint8_t i = 0; i < 32; i++) p->r[i] = 0;
    p->sp = 0x20 + 0x40 + sizeof(p->ram) - 1;
    p->sreg.data = 0;
    for (uint8_t i = 0; i < 128; i++) p->ram[i] = 0;
    p->pgm_address = pgm_address;
    p->clock_ticks = 0;
}

uint8_t processor_read(Processor *p, uint16_t addr) {
    uint8_t data;
    if (addr < 0x20) data = p->r[addr];
    else if (addr == 0x20 + 0x3d) data = p->sp & 0xff;
    else if (addr == 0x20 + 0x3e) data = (p->sp >> 8) &0b11;
    else if (addr == 0x20 + 0x3f) data = p->sreg.data;
    else if (addr >= 0x20 + 0x40 && (uint16_t)(addr - 0x20 - 0x40) < sizeof(p->ram)) data = p->ram[addr - 0x20 - 0x40];
    else data = 0;
    if (p->debug) printf_P(PSTR("READ mem[0x%04x] = %02x (%c)\n"), addr, data, (data >= ' ' && data <= '~') ? data : '.');
    return data;
}

void processor_write(Processor *p, uint16_t addr, uint8_t data) {
    if (addr < 0x20) p->r[addr] = data;
    if (addr == 0x20 + 0x3d) p->sp = (p->sp & 0b1100000000) | data;
    if (addr == 0x20 + 0x3e) p->sp = ((data & 0b11) << 8) | (p->sp & 0xff);
    if (addr == 0x20 + 0x3f) p->sreg.data = data;
    if (addr >= 0x20 + 0x40 && (uint16_t)(addr - 0x20 - 0x40) < sizeof(p->ram)) p->ram[addr - 0x20 - 0x40] = data;
    if (p->debug) printf_P(PSTR("WRITE mem[0x%04x] = %02x (%c)\n"), addr, data, (data >= ' ' && data <= '~') ? data : '.');
}

void processor_flags(Processor *p, uint8_t data, bool sub_zero_carry, bool overflow) {
    if (sub_zero_carry) {
        if (data != 0) {
            p->sreg.flags.z = false;
        }
    } else {
        p->sreg.flags.z = data == 0;
    }
    p->sreg.flags.n = bit(data, 7);
    p->sreg.flags.v = overflow;
    p->sreg.flags.s = p->sreg.flags.n != p->sreg.flags.v;
}

bool processor_add(Processor *p, uint8_t a, uint8_t b, bool carry_in, uint8_t *c) {
    #ifdef __builtin_add_overflow
        bool carry_out = __builtin_add_overflow(a, b + carry_in, c);
    #else
        *c = a + (b + carry_in);
        bool carry_out = (int16_t)a + (int16_t)(b + carry_in) > (int16_t)0xff;
    #endif
    processor_flags(p, *c, false, carry_in != carry_out);
    return carry_out;
}
void processor_add_with_carry(Processor *p, uint8_t a, uint8_t b, bool carry_in, uint8_t *c) {
    p->sreg.flags.c = processor_add(p, a, b, carry_in, c);
}
void processor_add_with_half_carry(Processor *p, uint8_t a, uint8_t b, bool carry_in, uint8_t *c) {
    p->sreg.flags.h = (int8_t)(a & 0b1111) + (int8_t)((b & 0b1111) + carry_in) > (int8_t)0xf;
    processor_add_with_carry(p, a, b, carry_in, c);
}

bool processor_sub(Processor *p, uint8_t a, uint8_t b, bool carry_in, uint8_t *c, bool zero_carry) {
    #ifdef __builtin_sub_overflow
        bool carry_out = __builtin_sub_overflow(a, b + carry_in, c);
    #else
        *c = a - (b + carry_in);
        bool carry_out = (int16_t)a - (int16_t)(b + carry_in) < (int16_t)0;
    #endif
    processor_flags(p, *c, zero_carry, carry_in != carry_out);
    return carry_out;
}
void processor_sub_with_carry(Processor *p, uint8_t a, uint8_t b, bool carry_in, uint8_t *c, bool zero_carry) {
    p->sreg.flags.c = processor_sub(p, a, b, carry_in, c, zero_carry);
}
void processor_sub_with_half_carry(Processor *p, uint8_t a, uint8_t b, bool carry_in, uint8_t *c, bool zero_carry) {
    p->sreg.flags.h = (int8_t)(a & 0b1111) - (int8_t)((b & 0b1111) + carry_in) < (int8_t)0;
    processor_sub_with_carry(p, a, b, carry_in, c, zero_carry);
}

const PROGMEM char output_string[] = "OUTPUT: ";

ProcessorState processor_clock(Processor *p) {
    if (!p->running) return PROCESSOR_STATE_HALTED;

    uint16_t i = eeprom_read_word(p->pgm_address + p->pc);

    uint16_t *X = (uint16_t *)&p->r[26];
    uint16_t *Y = (uint16_t *)&p->r[28];
    uint16_t *Z = (uint16_t *)&p->r[30];

    if (p->debug) {
        printf_P(PSTR("%04d pc:%04x regs:"), p->clock_ticks++, p->pc);
        for (uint8_t i = 0; i < 26; i++) {
            printf_P(PSTR("%02x "), p->r[i]);
        }
        printf_P(PSTR("X:%04x Y:%04x Z:%04x sp:%04x "), *X, *Y, *Z, p->sp);
        printf_P(PSTR("sreg:%c%c%c%c%c%c%c%c |"),
            p->sreg.flags.i ? 'I' : '-',
            p->sreg.flags.t ? 'T' : '-',
            p->sreg.flags.h ? 'H' : '-',
            p->sreg.flags.s ? 'S' : '-',
            p->sreg.flags.v ? 'V' : '-',
            p->sreg.flags.n ? 'N' : '-',
            p->sreg.flags.z ? 'Z' : '-',
            p->sreg.flags.c ? 'C' : '-');
        printf_P(PSTR(" %02x %02x  "), i & 0xff, i >> 8);
    }

    // ###############################################################################
    // ########################## SPECIAL FUNCTION VECTORS ###########################
    // ###############################################################################

    if (p->pc >= 2 && p->pc <= 26) {
        // ### Serial API ###

        // serial_write
        if (p->pc == 2) {
            char character = p->r[24];
            if (p->debug) printf_P(PSTR("serial_write(0x%02x)\n"), character);

            if (p->debug) serial_print_P(output_string);
            serial_write(character);
            if (p->debug) serial_write('\n');
        }

        // serial_print
        if (p->pc == 4) {
            uint16_t string = (p->r[25] << 8) | p->r[24];
            if (p->debug) printf_P(PSTR("serial_print(0x%04x)\n"), string);

            if (p->debug) serial_print_P(output_string);
            serial_print((char *)&p->ram[string - 0x20 - 0x40]);
            if (p->debug) serial_write('\n');
        }

        // serial_print_P
        if (p->pc == 6) {
            uint16_t string = (p->r[25] << 8) | p->r[24];
            if (p->debug) printf_P(PSTR("serial_print_P(0x%04x)\n"), string);

            if (p->debug) serial_print_P(output_string);
            uint16_t position = p->pgm_address + string;
            char character;
            while ((character = eeprom_read_byte(position++)) != '\0') {
                serial_write(character);
            }
            if (p->debug) serial_write('\n');
        }

        // serial_println
        if (p->pc == 8) {
            uint16_t string = (p->r[25] << 8) | p->r[24];
            if (p->debug) printf_P(PSTR("serial_println(0x%04x)\n"), string);

            if (p->debug) serial_print_P(output_string);
            serial_println((char *)&p->ram[string - 0x20 - 0x40]);
        }

        // serial_println_P
        if (p->pc == 10) {
            uint16_t string = (p->r[25] << 8) | p->r[24];
            if (p->debug) printf_P(PSTR("serial_println_P(0x%04x)\n"), string);

            if (p->debug) serial_print_P(output_string);
            uint16_t position = p->pgm_address + string;
            char character;
            while ((character = eeprom_read_byte(position++)) != '\0') {
                serial_write(character);
            }
            serial_write('\n');
        }

        // ### File API ###

        // file_open
        if (p->pc == 12) {
            uint16_t file_name = (p->r[25] << 8) | p->r[24];
            uint8_t file_mode = p->r[22];
            if (p->debug) printf_P(PSTR("file_open(0x%04x, %d)\n"), file_name, file_mode);

            p->r[24] = file_open((char *)&p->ram[file_name - 0x20 - 0x40], file_mode);
        }

        // file_name
        if (p->pc == 14) {
            int8_t file = p->r[24];
            uint16_t buffer = (p->r[23] << 8) | p->r[22];
            if (p->debug) printf_P(PSTR("file_name(%d, 0x%04x)\n"), file, buffer);

            p->r[24] = file_name(file, (char *)&p->ram[buffer - 0x20 - 0x40]);
        }

        // file_size
        if (p->pc == 16) {
            int8_t file = p->r[24];
            if (p->debug) printf_P(PSTR("file_size(%d)\n"), file);

            int16_t size = file_size(file);
            p->r[24] = size & 0xff;
            p->r[25] = size >> 8;
        }

        // file_position
        if (p->pc == 18) {
            int8_t file = p->r[24];
            if (p->debug) printf_P(PSTR("file_position(%d)\n"), file);

            int16_t position = file_position(file);
            p->r[24] = position & 0xff;
            p->r[25] = position >> 8;
        }

        // file_seek
        if (p->pc == 20) {
            int8_t file = p->r[24];
            int16_t position = (p->r[23] << 8) | p->r[22];
            if (p->debug) printf_P(PSTR("file_name(%d, 0x%04x)\n"), file, position);

            p->r[24] = file_seek(file, position);
        }

        // file_read
        if (p->pc == 22) {
            int8_t file = p->r[24];
            uint16_t buffer = (p->r[23] << 8) | p->r[22];
            uint16_t size = (p->r[21] << 8) | p->r[20];
            if (p->debug) printf_P(PSTR("file_read(%d, 0x%04x, 0x%04x)\n"), file, buffer, size);

            int16_t bytes_read = file_read(file, &p->ram[buffer - 0x20 - 0x40], size);
            p->r[24] = bytes_read & 0xff;
            p->r[25] = bytes_read >> 8;
        }

        // file_write
        if (p->pc == 24) {
            int8_t file = p->r[24];
            uint16_t buffer = (p->r[23] << 8) | p->r[22];
            uint16_t size = (p->r[21] << 8) | p->r[20];
            if (p->debug) printf_P(PSTR("file_write(%d, 0x%04x, 0x%04x)\n"), file, buffer, size);

            int16_t bytes_written = file_write(file, &p->ram[buffer - 0x20 - 0x40], size);
            p->r[24] = bytes_written & 0xff;
            p->r[25] = bytes_written >> 8;
        }

        // file_close
        if (p->pc == 26) {
            int8_t file = p->r[24];
            if (p->debug) printf_P(PSTR("file_close(%d)\n"), file);

            p->r[24] = file_close(file);
        }

        p->pc = processor_read(p, ++p->sp);
        p->pc |= (processor_read(p, ++p->sp) << 8);
        return PROCESSOR_STATE_RETURN;
    }

    // ###############################################################################
    // ############################# INSTRUCTION DECODING ############################
    // ###############################################################################

    p->pc += 2;

    // Registers
    uint8_t Rd = (i >> 4) & 0b11111; // register data: 0000 000d dddd 0000
    uint8_t Rdu = ((i >> 4) & 0b1111) + 16; // register data upper: 0000 0000 dddd 0000
    uint8_t Rdw = ((i >> 4) & 0b1111) << 1; // register data word: 0000 0000 dddd 0000
    uint8_t Rdwp = (((i >> 4) & 0b11) << 1) + 24; // register data word pair: 0000 0000 00dd 0000
    uint8_t Rr = (bit(i, 9) << 4) | (i & 0b1111); // register read: 0000 00r0 0000 rrrr
    uint8_t Rrw = (i & 0b1111) << 1; // register read word: 0000 0000 0000 rrrr

    // Data
    uint8_t K = ((i >> 4) & 0b11110000) | (i & 0b1111); // data: 0000 KKKK 0000 KKKK
    uint8_t K6 = ((i >> 2) & 0b110000) | (i & 0b1111); // data 6 bits: 0000 0000 KK00 KKKK

    // Addresses
    int16_t k = (i & 0b111111111111) << 1; // pgm address: 0000 kkkk kkkk kkkk
    if (bit(k, 12)) k |= 0b1110000000000000;
    int16_t k7 = (i >> 2) & 0b11111110; // pgm address: 0000 00kk kkkk k000
    if (bit(k7, 7)) k7 |= 0b1111111110000000;

    // IO registers
    uint8_t A = ((i >> 5) & 0b110000) | (i & 0b1111); // io register: 0000 0AA0 0000 AAAA
    uint8_t Al = (i >> 3) & 0b11111; // io register: 0000 0000 AAAA A000

    // Bits
    uint8_t b = i & 0b111; // bit: 0000 0000 0000 0bbb
    uint8_t s = (i >> 4) & 0b111; // bit: 0000 0000 0sss 0000

    // ###############################################################################
    // ###################### ARITHMETIC AND LOGIC INSTRUCTIONS ######################
    // ###############################################################################

    // add Rd, Rr | adc Rd, Rr | 000c 11rd dddd rrrr
    if ((i & 0b1110110000000000) == 0b0000110000000000) {
        bool carry = bit(i, 12);

        if (Rd == Rr) {
            // lsl Rd | 0000 11dd dddd dddd
            if (!carry) {
                if (p->debug) printf_P(PSTR("lsl r%d (0x%02x)\n"), Rd, p->r[Rd]);
                p->sreg.flags.c = bit(p->r[Rd], 7);
                p->r[Rd] <<= 1;

                p->sreg.flags.n = bit(p->r[Rd], 7);
                processor_flags(p, p->r[Rd], false, p->sreg.flags.n != p->sreg.flags.c);
                return PROCESSOR_STATE_NORMAL;
            }

            // rol Rd | 0001 11dd dddd dddd
            if (p->debug) printf_P(PSTR("rol r%d (0x%02x)\n"), Rd, p->r[Rd]);
            bool old_carry = p->sreg.flags.c;
            p->sreg.flags.c = bit(p->r[Rd], 7);
            p->r[Rd] <<= 1;
            p->r[Rd] |= old_carry;

            p->sreg.flags.n = bit(p->r[Rd], 7);
            processor_flags(p, p->r[Rd], false, p->sreg.flags.n != p->sreg.flags.c);
            return PROCESSOR_STATE_NORMAL;
        }

        if (p->debug) printf_P(PSTR("%" PRIpstr " r%d (0x%02x), r%d (0x%02x)\n"), carry ? PSTR("adc") : PSTR("add"), Rd, p->r[Rd], Rr, p->r[Rr]);
        processor_add_with_half_carry(p, p->r[Rd], p->r[Rr], carry ? p->sreg.flags.c : false, &p->r[Rd]);
        return PROCESSOR_STATE_NORMAL;
    }

    // adiw Rd, K | 1001 0110 KKdd KKKK
    if ((i & 0b1111111100000000) == 0b1001011000000000) {
        if (p->debug) printf_P(PSTR("adiw r%d (0x%02x%02x), 0x%02x\n"), Rdwp, p->r[Rdwp + 1], p->r[Rdwp], K6);
        processor_add_with_carry(p, p->r[Rdwp], K6, false, &p->r[Rdwp]);
        processor_add_with_carry(p, p->r[Rdwp + 1], 0, p->sreg.flags.c, &p->r[Rdwp + 1]);
        return PROCESSOR_STATE_NORMAL;
    }

    // sub Rd, Rr | sbc Rd, Rr | 000c 10rd dddd rrrr
    if ((i & 0b1110110000000000) == 0b0000100000000000) {
        bool carry = bit(i, 12) == 0;
        if (p->debug) printf_P(PSTR("%" PRIpstr " r%d (0x%02x), r%d (0x%02x)\n"), carry ? PSTR("sbc") : PSTR("sub"), Rd, p->r[Rd], Rr, p->r[Rr]);
        processor_sub_with_half_carry(p, p->r[Rd], p->r[Rr], carry ? p->sreg.flags.c : false, &p->r[Rd], carry);
        return PROCESSOR_STATE_NORMAL;
    }

    // subi Rd, K | sbci Rd, K | 010c KKKK dddd KKKK
    if ((i & 0b1110000000000000) == 0b0100000000000000) {
        bool carry = bit(i, 12) == 0;
        if (p->debug) printf_P(PSTR("%" PRIpstr " r%d (0x%02x), 0x%02x\n"), carry ? PSTR("sbci") : PSTR("subi"), Rdu, p->r[Rdu], K);
        processor_sub_with_half_carry(p, p->r[Rdu], K, carry ? p->sreg.flags.c : false, &p->r[Rdu], carry);
        return PROCESSOR_STATE_NORMAL;
    }

    // sbiw Rd, K | 1001 0111 KKdd KKKK
    if ((i & 0b1111111100000000) == 0b1001011100000000) {
        if (p->debug) printf_P(PSTR("sbiw r%d (0x%02x%02x), 0x%02x\n"), Rdwp, p->r[Rdwp + 1], p->r[Rdwp], K6);
        processor_sub_with_carry(p, p->r[Rdwp], K6, false, &p->r[Rdwp], false);
        processor_sub_with_carry(p, p->r[Rdwp + 1], 0, p->sreg.flags.c, &p->r[Rdwp + 1], false);
        return PROCESSOR_STATE_NORMAL;
    }

    // and Rd, Rr | 0010 00rd dddd rrrr
    if ((i & 0b1111110000000000) == 0b0010000000000000) {
        if (p->debug) printf_P(PSTR("and r%d (0x%02x), r%d (0x%02x)\n"), Rd, p->r[Rd], Rr, p->r[Rr]);
        p->r[Rd] &= p->r[Rr];
        processor_flags(p, p->r[Rd], false, false);
        return PROCESSOR_STATE_NORMAL;
    }

    // andi Rd, K | 0111 KKKK dddd KKKK
    if ((i & 0b1111000000000000) == 0b0111000000000000) {
        if (p->debug) printf_P(PSTR("andi r%d (0x%02x), 0x%02x\n"), Rdu, p->r[Rdu], K);
        p->r[Rdu] &= K;
        processor_flags(p, p->r[Rdu], false, false);
        return PROCESSOR_STATE_NORMAL;
    }

    // or Rd, Rr | 0010 10rd dddd rrrr
    if ((i & 0b1111110000000000) == 0b0101000000000000) {
        if (p->debug) printf_P(PSTR("or r%d (0x%02x), r%d (0x%02x)\n"), Rd, p->r[Rd], Rr, p->r[Rr]);
        p->r[Rd] |= p->r[Rr];
        processor_flags(p, p->r[Rd], false, false);
        return PROCESSOR_STATE_NORMAL;
    }

    // ori Rd, K | 0110 KKKK dddd KKKK
    if ((i & 0b1111000000000000) == 0b0110000000000000) {
        if (p->debug) printf_P(PSTR("ori r%d (0x%02x), 0x%02x\n"), Rdu, p->r[Rdu], K);
        p->r[Rdu] |= K;
        processor_flags(p, p->r[Rdu], false, false);
        return PROCESSOR_STATE_NORMAL;
    }

    // eor Rd, Rr | 0010 01rd dddd rrrr
    if ((i & 0b1111110000000000) == 0b0010010000000000) {
        if (p->debug) printf_P(PSTR("eor r%d (0x%02x), r%d (0x%02x)\n"), Rd, p->r[Rd], Rr, p->r[Rr]);
        p->r[Rd] ^= p->r[Rr];
        processor_flags(p, p->r[Rd], false, false);
        return PROCESSOR_STATE_NORMAL;
    }

    // com Rd | 1001 010d dddd 0000
    if ((i & 0b1111111000001111) == 0b1001010000000000) {
        if (p->debug) printf_P(PSTR("com r%d (0x%02x)\n"), Rd, p->r[Rd]);
        processor_sub_with_carry(p, 0xff, p->r[Rd], false, &p->r[Rd], false);
        return PROCESSOR_STATE_NORMAL;
    }

    // neg Rd | 1001 010d dddd 0001
    if ((i & 0b1111111000001111) == 0b1001010000000001) {
        if (p->debug) printf_P(PSTR("neg r%d (0x%02x)\n"), Rd, p->r[Rd]);
        processor_sub_with_half_carry(p, 0, p->r[Rd], false, &p->r[Rd], false);
        return PROCESSOR_STATE_NORMAL;
    }

    // inc Rd | 1001 010d dddd 0011
    if ((i & 0b1111111000001111) == 0b1001010000000011) {
        if (p->debug) printf_P(PSTR("inc r%d (0x%02x)\n"), Rd, p->r[Rd]);
        processor_add(p, p->r[Rd], 1, false, &p->r[Rd]);
        return PROCESSOR_STATE_NORMAL;
    }

    // dec Rd | 1001 010d dddd 1010
    if ((i & 0b1111111000001111) == 0b1001010000001010) {
        if (p->debug) printf_P(PSTR("dec r%d (0x%02x)\n"), Rd, p->r[Rd]);
        processor_sub(p, p->r[Rd], 1, false, &p->r[Rd], false);
        return PROCESSOR_STATE_NORMAL;
    }

    // ###############################################################################
    // ############################# BRANCH INSTRUCTIONS #############################
    // ###############################################################################

    // rjmp k | 1100 kkkk kkkk kkkk
    if ((i & 0b1111000000000000) == 0b1100000000000000) {
        if (p->debug) printf_P(PSTR("rjmp %+d\n"), k);
        if (k == -2) p->running = false;
        p->pc += k;
        return PROCESSOR_STATE_NORMAL;
    }

    // ijmp | 1001 0100 0000 1001
    if (i == 0b1001010000001001) {
        if (p->debug) printf_P(PSTR("ijmp (0x%04x)"), *Z);
        p->pc = *Z;
        return PROCESSOR_STATE_NORMAL;
    }

    // Instruction: JMP
    // Encoding: 1001 010k kkkk 110k
    // Encoding: kkkk kkkk kkkk kkkk

    // rcall | 1101 kkkk kkkk kkkk
    if ((i & 0b1111000000000000) == 0b1101000000000000) {
        if (p->debug) printf_P(PSTR("rcall %+d\n"), k);
        processor_write(p, p->sp--, p->pc >> 8);
        processor_write(p, p->sp--, p->pc & 0xff);
        p->pc += k;
        return PROCESSOR_STATE_CALL;
    }

    // icall | 1001 0101 0000 1001
    if (i == 0b1001010100001001) {
        if (p->debug) printf_P(PSTR("icall (0x%04x)"), *Z);
        processor_write(p, p->sp--, p->pc >> 8);
        processor_write(p, p->sp--, p->pc & 0xff);
        p->pc = *Z;
        return PROCESSOR_STATE_CALL;
    }

    // Instruction: CALL
    // Encoding: 1001 010k kkkk 111k
    // Encoding: kkkk kkkk kkkk kkkk

    // ret | reti | 1001 0101 000i 1000
    if ((i & 0b1111111111101111) == 0b1001010100001000) {
        bool interrupt = bit(i, 4);
        if (p->debug) printf_P(PSTR("%" PRIpstr "\n"), interrupt ? PSTR("iret") : PSTR("ret"));
        p->pc = processor_read(p, ++p->sp);
        p->pc |= (processor_read(p, ++p->sp) << 8);
        if (interrupt) p->sreg.flags.i = true;
        return PROCESSOR_STATE_RETURN;
    }

    // cpse Rd, Rr | 0001 00rd dddd rrrr
    if ((i & 0b1111110000000000) == 0b0001000000000000) {
        if (p->debug) printf_P(PSTR("cpse r%d (0x%02x), r%d (0x%02x)\n"), Rd, p->r[Rd], Rr, p->r[Rr]);
        if (p->r[Rd] == p->r[Rr]) p->pc += 2;
        return PROCESSOR_STATE_NORMAL;
    }

    // cp Rd, Rr | cpc Rd, Rr | 000c 01rd dddd rrrr
    if ((i & 0b1110110000000000) == 0b0000010000000000) {
        bool carry = bit(i, 12) == 0;
        if (p->debug) printf_P(PSTR("%" PRIpstr " r%d (0x%02x), r%d (0x%02x)\n"), carry ? PSTR("cpc") : PSTR("cp"), Rd, p->r[Rd], Rr, p->r[Rr]);
        uint8_t c;
        processor_sub_with_half_carry(p, p->r[Rd], p->r[Rr], carry ? p->sreg.flags.c : false, &c, carry);
        return PROCESSOR_STATE_NORMAL;
    }

    // cpi Rd, K | 0011 KKKK dddd KKKK
    if ((i & 0b1111000000000000) == 0b0011000000000000) {
        if (p->debug) printf_P(PSTR("cpi r%d (0x%02x), 0x%02x\n"), Rdu, p->r[Rdu], K);
        uint8_t c;
        processor_sub_with_half_carry(p, p->r[Rdu], K, false, &c, false);
        return PROCESSOR_STATE_NORMAL;
    }

    // sbrs Rr, b | sbrc Rr, b | 1111 11sd dddd 0bbb
    if ((i & 0b1111110000001000) == 0b1111110000000000) {
        bool set = bit(i, 9) == 0;
        if (p->debug) printf_P(PSTR("%" PRIpstr " r%d (0x%02x), %d\n"), set ? PSTR("sbrs") : PSTR("sbrc"), Rd, p->r[Rd], b);
        if (bit(p->r[Rd], b) == set) p->pc += 2;
        return PROCESSOR_STATE_NORMAL;
    }

    // sbis A, b | sbic A, b | 1001 10s1 AAAA Abbb
    if ((i & 0b1111110100000000) == 0b1001100100000000) {
        bool set = bit(i, 9) == 0;
        if (p->debug) printf_P(PSTR("%" PRIpstr " 0x%02x, %d\n"), set ? PSTR("sbis") : PSTR("sbic"), Al, b);
        if (bit(processor_read(p, 0x20 + Al), b) == set) p->pc += 2;
        return PROCESSOR_STATE_NORMAL;
    }

    // brbs s, k | brbc s, k | 1111 0skk kkkk kbbb
    if ((i & 0b1111100000000000) == 0b1111000000000000) {
        bool set = bit(i, 10) == 0;
        if (p->debug) printf_P(PSTR("%" PRIpstr " %d (%c), %+d\n"), set ? PSTR("brbs") : PSTR("brbc"), b, pgm_read_byte(PSTR("CZNVSHTI") + b), k7);
        if (bit(p->sreg.data, b) == set) p->pc += k7;
        return PROCESSOR_STATE_NORMAL;
    }

    // ###############################################################################
    // ######################### DATA TRANSFER INSTRUCTIONS ##########################
    // ###############################################################################

    // mov Rd, Rr | 0010 11rd dddd rrrr
    if ((i & 0b1111110000000000) == 0b0010110000000000) {
        if (p->debug) printf_P(PSTR("mov r%d, r%d (0x%02x)\n"), Rd, Rr, p->r[Rr]);
        p->r[Rd] = p->r[Rr];
        return PROCESSOR_STATE_NORMAL;
    }

    // movw Rd, Rr | 0000 0001 dddd rrrr
    if ((i & 0b1111111100000000) == 0b0000000100000000) {
        if (p->debug) printf_P(PSTR("movw r%d, r%d (0x%02x%02x)\n"), Rdw, Rrw, p->r[Rrw + 1], p->r[Rrw]);
        p->r[Rdw] = p->r[Rrw];
        p->r[Rdw + 1] = p->r[Rrw + 1];
        return PROCESSOR_STATE_NORMAL;
    }

    // ldi Rdu, K | 1110 KKKK dddd KKKK
    if ((i & 0b1111000000000000) == 0b1110000000000000) {
        if (p->debug) printf_P(PSTR("ldi r%d, 0x%02x\n"), Rdu, K);
        p->r[Rdu] = K;
        return PROCESSOR_STATE_NORMAL;
    }

    // ld Rd, X | 1001 000d dddd 1100
    if ((i & 0b1111111000001111) == 0b1001000000001100) {
        if (p->debug) printf_P(PSTR("ld r%d, X (0x%04x)\n"), Rd, *X);
        p->r[Rd] = processor_read(p, *X);
        return PROCESSOR_STATE_NORMAL;
    }

    // ld Rd, X+ | 1001 000d dddd 1101
    if ((i & 0b1111111000001111) == 0b1001000000001101) {
        if (p->debug) printf_P(PSTR("ld r%d, X+ (0x%04x)\n"), Rd, *X);
        p->r[Rd] = processor_read(p, *X);
        (*X)++;
        return PROCESSOR_STATE_NORMAL;
    }

    // ld Rd, -X | 1001 000d dddd 1110
    if ((i & 0b1111111000001111) == 0b1001000000001110) {
        (*X)--;
        if (p->debug) printf_P(PSTR("ld r%d, -X (0x%04x)\n"), Rd, *X);
        p->r[Rd] = processor_read(p, *X);
        return PROCESSOR_STATE_NORMAL;
    }

    // ld Rd, Y | 1000 000d dddd 1000
    if ((i & 0b1111111000001111) == 0b1000000000001000) {
        if (p->debug) printf_P(PSTR("ld r%d, Y (0x%04x)\n"), Rd, *Y);
        p->r[Rd] = processor_read(p, *Y);
        return PROCESSOR_STATE_NORMAL;
    }

    // ld Rd, Y+ | 1001 000d dddd 1001
    if ((i & 0b1111111000001111) == 0b1001000000001001) {
        if (p->debug) printf_P(PSTR("ld r%d, Y+ (0x%04x)\n"), Rd, *Y);
        p->r[Rd] = processor_read(p, *Y);
        (*Y)++;
        return PROCESSOR_STATE_NORMAL;
    }

    // ld Rd, -Y | 1001 000d dddd 1010
    if ((i & 0b1111111000001111) == 0b1001000000001010) {
        (*Y)--;
        if (p->debug) printf_P(PSTR("ld r%d, -Y (0x%04x)\n"), Rd, *Y);
        p->r[Rd] = processor_read(p, *Y);
        return PROCESSOR_STATE_NORMAL;
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

    // ld Rd, Z | 1000 000d dddd 0000
    if ((i & 0b1111111000001111) == 0b1000000000000000) {
        if (p->debug) printf_P(PSTR("ld r%d, Z (0x%04x)\n"), Rd, *Z);
        p->r[Rd] = processor_read(p, *Z);
        return PROCESSOR_STATE_NORMAL;
    }

    // ld Rd, Z+ | 1001 000d dddd 0001
    if ((i & 0b1111111000001111) == 0b1001000000000001) {
        if (p->debug) printf_P(PSTR("ld r%d, Z+ (0x%04x)\n"), Rd, *Z);
        p->r[Rd] = processor_read(p, *Z);
        (*Z)++;
        return PROCESSOR_STATE_NORMAL;
    }

    // ld Rd, -Z | 1001 000d dddd 0010
    if ((i & 0b1111111000001111) == 0b1001000000000010) {
        (*Z)--;
        if (p->debug) printf_P(PSTR("ld r%d, -Z (0x%04x)\n"), Rd, *Z);
        p->r[Rd] = processor_read(p, *Z);
        return PROCESSOR_STATE_NORMAL;
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

    // st X, Rd | 1001 001d dddd 1100
    if ((i & 0b1111111000001111) == 0b1001001000001100) {
        if (p->debug) printf_P(PSTR("st X (0x%04x), r%d (0x%02x)\n"), *X, Rd, p->r[Rd]);
        processor_write(p, *X, p->r[Rd]);
        return PROCESSOR_STATE_NORMAL;
    }

    // st X+, Rd | 1001 001d dddd 1101
    if ((i & 0b1111111000001111) == 0b1001001000001101) {
        if (p->debug) printf_P(PSTR("st X+ (0x%04x), r%d (0x%02x)\n"), *X, Rd, p->r[Rd]);
        processor_write(p, *X, p->r[Rd]);
        (*X)++;
        return PROCESSOR_STATE_NORMAL;
    }

    // st -X, Rd | 1001 001d dddd 1110
    if ((i & 0b1111111000001111) == 0b1001001000001110) {
        (*X)--;
        if (p->debug) printf_P(PSTR("st -X (0x%04x), r%d (0x%02x)\n"), *X, Rd, p->r[Rd]);
        processor_write(p, *X, p->r[Rd]);
        return PROCESSOR_STATE_NORMAL;
    }

    // st Y, Rd | 1000 001d dddd 1000
    if ((i & 0b1111111000001111) == 0b1000001000001000) {
        if (p->debug) printf_P(PSTR("st Y (0x%04x), r%d (0x%02x)\n"), *Y, Rd, p->r[Rd]);
        processor_write(p, *Y, p->r[Rd]);
        return PROCESSOR_STATE_NORMAL;
    }

    // st Y+, Rd | 1001 001d dddd 1001
    if ((i & 0b1111111000001111) == 0b1001001000001001) {
        if (p->debug) printf_P(PSTR("st Y+ (0x%04x), r%d (0x%02x)\n"), *Y, Rd, p->r[Rd]);
        processor_write(p, *Y, p->r[Rd]);
        (*Y)++;
        return PROCESSOR_STATE_NORMAL;
    }

    // st -Y, Rd | 1001 001d dddd 1010
    if ((i & 0b1111111000001111) == 0b1001001000001010) {
        (*Y)--;
        if (p->debug) printf_P(PSTR("st -Y (0x%04x), r%d (0x%02x)\n"), *Y, Rd, p->r[Rd]);
        processor_write(p, *Y, p->r[Rd]);
        return PROCESSOR_STATE_NORMAL;
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

    // st Z, Rd | 1000 001d dddd 0000
    if ((i & 0b1111111000001111) == 0b1000001000000000) {
        if (p->debug) printf_P(PSTR("st Z (0x%04x), r%d (0x%02x)\n"), *Z, Rd, p->r[Rd]);
        processor_write(p, *Z, p->r[Rd]);
        return PROCESSOR_STATE_NORMAL;
    }

    // st Z+, Rd | 1001 001d dddd 0001
    if ((i & 0b1111111000001111) == 0b1001001000000001) {
        if (p->debug) printf_P(PSTR("st Z+ (0x%04x), r%d (0x%02x)\n"), *Z, Rd, p->r[Rd]);
        processor_write(p, *Z, p->r[Rd]);
        (*Z)++;
        return PROCESSOR_STATE_NORMAL;
    }

    // st -Z, Rd | 1001 001d dddd 0010
    if ((i & 0b1111111000001111) == 0b1001001000000010) {
        (*Z)--;
        if (p->debug) printf_P(PSTR("st -Z (0x%04x), r%d (0x%02x)\n"), *Z, Rd, p->r[Rd]);
        processor_write(p, *Z, p->r[Rd]);
        return PROCESSOR_STATE_NORMAL;
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

    // lpm r0, Z | 1001 0101 1100 1000
    if (i == 0b1001010111001000) {
        if (p->debug) printf_P(PSTR("lpm Z (0x%04x)\n"), *Z);
        p->r[0] = eeprom_read_byte(p->pgm_address + *Z);
        return PROCESSOR_STATE_NORMAL;
    }

    // lpm Rd, Z | 1001 000d dddd 0100
    if ((i & 0b1111111000001111) == 0b1001000000000100) {
        if (p->debug) printf_P(PSTR("lpm r%d, Z (0x%04x)\n"), Rd, *Z);
        p->r[Rd] = eeprom_read_byte(p->pgm_address + *Z);
        return PROCESSOR_STATE_NORMAL;
    }

    // lpm Rd, Z+ | 1001 000d dddd 0101
    if ((i & 0b1111111000001111) == 0b1001000000000101) {
        if (p->debug) printf_P(PSTR("lpm r%d, Z+ (0x%04x)\n"), Rd, *Z);
        p->r[Rd] = eeprom_read_byte(p->pgm_address + *Z);
        (*Z)++;
        return PROCESSOR_STATE_NORMAL;
    }

    // in Rd, A | 1011 0AAd dddd AAAA
    if ((i & 0b1111100000000000) == 0b1011000000000000) {
        if (p->debug) printf_P(PSTR("in r%d, 0x%02x\n"), Rd, A);
        p->r[Rd] = processor_read(p, 0x20 + A);
        return PROCESSOR_STATE_NORMAL;
    }

    // out A, Rr | 1011 1AAd dddd AAAA
    if ((i & 0b1111100000000000) == 0b1011100000000000) {
        if (p->debug) printf_P(PSTR("out 0x%02x, r%d (0x%02x)\n"), A, Rd, p->r[Rd]);
        processor_write(p, 0x20 + A, p->r[Rd]);
        return PROCESSOR_STATE_NORMAL;
    }

    // push Rd | 1001 001d dddd 1111
    if ((i & 0b1111111000001111) == 0b1001001000001111) {
        if (p->debug) printf_P(PSTR("push r%d (0x%02x)\n"), Rd, p->r[Rd]);
        processor_write(p, p->sp--, p->r[Rd]);
        return PROCESSOR_STATE_NORMAL;
    }

    // pop Rd | 1001 000d dddd 1111
    if ((i & 0b1111111000001111) == 0b1001000000001111) {
        if (p->debug) printf_P(PSTR("pop r%d\n"), Rd);
        p->r[Rd] = processor_read(p, ++p->sp);
        return PROCESSOR_STATE_NORMAL;
    }

    // ###############################################################################
    // ####################### BIT AND BIT-TEST INSTRUCTIONS #########################
    // ###############################################################################

    // sbi A, b | cbi A, b | 1001 10s0 AAAA Abbb
    if ((i & 0b1111110100000000) == 0b1001100000000000) {
        uint8_t set = bit(i, 9);
        if (p->debug) printf_P(PSTR("%" PRIpstr " 0x%02x, %d\n"), set ? PSTR("sbi") : PSTR("cbi"), Al, b);
        uint8_t data = processor_read(p, 0x20 + Al);
        if (set) {
            bit_set(data, b);
        } else {
            bit_clear(data, b);
        }
        processor_write(p, 0x20 + Al, data);
        return PROCESSOR_STATE_NORMAL;
    }

    // lsr Rd | 1001 010d dddd 0110
    if ((i & 0b1111111000001111) == 0b1001010000000110) {
        if (p->debug) printf_P(PSTR("lsr r%d (0x%02x)\n"), Rd, p->r[Rd]);
        p->sreg.flags.c = bit(p->r[Rd], 0);
        p->r[Rd] >>= 1;

        p->sreg.flags.n = bit(p->r[Rd], 7);
        processor_flags(p, p->r[Rd], false, p->sreg.flags.n != p->sreg.flags.c);
        return PROCESSOR_STATE_NORMAL;
    }

    // ror Rd | 1001 010d dddd 0111
    if ((i & 0b1111111000001111) == 0b1001010000000111) {
        if (p->debug) printf_P(PSTR("ror r%d (0x%02x)\n"), Rd, p->r[Rd]);
        bool old_carry = p->sreg.flags.c;
        p->sreg.flags.c = bit(p->r[Rd], 0);
        p->r[Rd] >>= 1;
        p->r[Rd] |= old_carry << 7;

        p->sreg.flags.n = bit(p->r[Rd], 7);
        processor_flags(p, p->r[Rd], false, p->sreg.flags.n != p->sreg.flags.c);
        return PROCESSOR_STATE_NORMAL;
    }

    // asr Rd | 1001 010d dddd 0101
    if ((i & 0b1111111000001111) == 0b1001010000000101) {
        if (p->debug) printf_P(PSTR("asr r%d (0x%02x)\n"), Rd, p->r[Rd]);
        bool old_top_bit = bit(p->r[Rd], 7);
        p->sreg.flags.c = bit(p->r[Rd], 0);
        p->r[Rd] >>= 1;
        if (old_top_bit) p->r[Rd] |= 1 << 7;

        p->sreg.flags.n = bit(p->r[Rd], 7);
        processor_flags(p, p->r[Rd], false, p->sreg.flags.n != p->sreg.flags.c);
        return PROCESSOR_STATE_NORMAL;
    }

    // swap Rd | 1001 010d dddd 0010
    if ((i & 0b1111111000001111) == 0b1001010000000010) {
        if (p->debug) printf_P(PSTR("Execute: swap r%d (0x%02x)\n"), Rd, p->r[Rd]);
        p->r[Rd] = ((p->r[Rd] & 0b1111) << 4) | (p->r[Rd] >> 4);
        return PROCESSOR_STATE_NORMAL;
    }

    // bset s | bclr s | 1001 0100 xsss 1000
    if ((i & 0b1111111100001111) == 0b1001010000001000) {
        uint8_t set = bit(i, 7) == 0;
        if (p->debug) printf_P(PSTR("%" PRIpstr " %d\n"), set ? PSTR("bset") : PSTR("bclr"), s);
        if (set) {
            bit_set(p->sreg.data, s);
        } else {
            bit_clear(p->sreg.data, s);
        }
        return PROCESSOR_STATE_NORMAL;
    }

    // bst Rd, b | 1111 101d dddd 0bbb
    if ((i & 0b1111111000001000) == 0b1111101000000000) {
        if (p->debug) printf_P(PSTR("bst r%d, %d\n"), Rd, b);
        p->sreg.flags.t = bit(p->r[Rd], b);
        return PROCESSOR_STATE_NORMAL;
    }

    // bld Rd, b | 1111 100d dddd 0bbb
    if ((i & 0b1111111000001000) == 0b1111100000000000) {
        if (p->debug) printf_P(PSTR("bld r%d, %d\n"), Rd, b);
        if (p->sreg.flags.t) {
            bit_set(p->r[Rd], b);
        } else {
            bit_clear(p->r[Rd], b);
        }
        return PROCESSOR_STATE_NORMAL;
    }

    // nop | sleep | wdr | break | spm
    // 0000 0000 0000 0000 | 1001 0101 1000 1000 | 1001 0101 1010 1000 | 1001 0101 1001 1000 | 1001 0101 1110 1000
    if (i == 0b0000000000000000 || i == 0b1001010110001000 || i == 0b1001010110101000 || i == 0b1001010110011000|| i == 0b1001010111101000) {
        printf_P(PSTR("nop\n"));
        return PROCESSOR_STATE_NORMAL;
    }

    printf_P(PSTR("Unkown instruction!\n"));
    p->running = false;
    return PROCESSOR_STATE_UNKOWN_INSTRUCTION;
}
