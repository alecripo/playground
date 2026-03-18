#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

char* REGISTERS_SMALL[8] = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};
char* REGISTERS_WIDE[8] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};
char* EFFECTIVE_ADDRESSES[8] = {"bx + si", "bx + di", "bp + si", "bp + di", "si", "di", "bp", "bx"};

int
handle_mov(int byte, FILE* infile, FILE* outfile) {
    char dest[32]="";
    char src[32]="";
    bool is_first_op_dest = byte & 0b00000010;
    char** register_names;
    // Are we working with wide or small registers?
    if (byte & 0b00000001) {
        register_names = REGISTERS_WIDE;
    } else {
        register_names = REGISTERS_SMALL;
    }

    // Layout:
    // _  _              _  _  _            _  _  _
    //|2 bits : mode|   |3 bits: register| |3 bits: register/memory|
    if ( (byte = fgetc(infile)) == EOF ) {
        fprintf(stderr, "Invalid byte stream: expected second byte of MOV, got EOF");
        return 1;
    }
    uint8_t mode = (byte >> 6);
    uint8_t reg = (byte & 0b00111000) >> 3;
    uint8_t reg_or_mem = (byte & 0b00000111);

    strncpy(dest, register_names[reg], 32);
    strncpy(src, register_names[reg_or_mem], 32);

    switch (mode) {
    case 0x00:
        if (reg_or_mem != 0b00000110) {
            sprintf(src, "[%s]", EFFECTIVE_ADDRESSES[reg_or_mem]);
            break;
        }
        int16_t imm_val = 0;
        byte = fgetc(infile);
        if (byte == EOF) {
            fprintf(stderr, "Invalid byte stream: expected immediate value, got EOF");
            return 1;
        }
        imm_val |= byte;
        byte = fgetc(infile);
        if (byte == EOF) {
            fprintf(stderr, "Invalid byte stream: expected immediate value, got EOF");
            return 1;
        }
        imm_val = ((int16_t)byte << 8) | imm_val;
        sprintf(src, "%d", imm_val);
        break;
    // handle 8-bit and 16-bit memory displacements in the same block
    case 0x01: /* FALLTHROUGH */
    case 0x02: {
        int16_t displacement = 0;
        if ((byte = fgetc(infile)) == EOF) {
            fprintf(stderr, "Invalid byte stream: expected 8-bit displacement, got EOF");
            return 1;
        }
        displacement |= byte;

        if (mode == 0x02) {
            byte = fgetc(infile);
            if (byte == EOF) {
                fprintf(stderr, "Invalid byte stream: expected 16-bit displacement, got EOF");
                return 1;
            }
            displacement = ((int16_t)byte<<8) | displacement;
        }

        if (displacement != 0) {
            sprintf(src, "[%s + %d]", EFFECTIVE_ADDRESSES[reg_or_mem], displacement);
        } else {
            sprintf(src, "[%s]", EFFECTIVE_ADDRESSES[reg_or_mem]);
        }
        break;
    }
    case 0x03: /* FALL THROUGH */
    default:
        break;
    }

    if (!is_first_op_dest) {
        char aux[32];
        strncpy(aux, dest, 32);
        strncpy(dest, src, 32);
        strncpy(src, aux, 32);
    }
    fprintf(outfile, "mov %s, %s\n", dest, src);
    return 0;

}
int
handle_immediate_mov(int op_byte, FILE* infile, FILE* outfile) {
    bool is_wide = op_byte & 0b00001000;
    uint8_t reg = op_byte & 0b00000111;
    int16_t immediate=0;

    if ( (op_byte = fgetc(infile)) == EOF ) {
        fprintf(stderr, "Invalid byte stream: expected second byte of MOV, got EOF");
        return 1;
    }
    immediate |= op_byte;

    if (!is_wide) {
        immediate = (immediate ^ 0x80)-0x80;
        fprintf(outfile, "mov %s, %d\n", REGISTERS_SMALL[reg], immediate);
        return 0;
    }

    if ( (op_byte = fgetc(infile)) == EOF ) {
        fprintf(stderr, "Invalid byte stream: expected second byte of MOV, got EOF");
        return 1;
    }
    int16_t high_byte = op_byte;
    immediate = high_byte<<8 | immediate;

    fprintf(outfile, "mov %s, %d\n", REGISTERS_WIDE[reg], immediate);
    return 0;
}

int
main(int argc, char** argv) {
    if (argc != 3) {
        printf("Usage: decoder <IN_FILE> <OUT_FILE>\n\
            <IN_FILE>: binary file that will be decoded to 8086 assembly\n\
            <OUTFILE>: file where output will be written. '-' writes to stdout\n");
        return 1;
    }
    FILE* infile = fopen(argv[1], "r");
    if (infile == NULL) {
        printf("Invalid input file: failed to open %s", argv[1]);
        return 1;
    }
    FILE *outfile;
    if (strncmp(argv[2], "-", 1) == 0) {
        outfile = stdout;
    } else {
        outfile = fopen(argv[2], "w+");
        if (outfile == NULL) {
            printf("Invalid output file: failed to open %s", argv[2]);
            return 1;
        }
    }

    int byte;
    loop:
    while (true) {
        byte = fgetc(infile);
        if (byte == EOF) {
            break;
        }

        /* Opcode Layout:
            _  _  _  _           _             _ _ _
           |4 bits : opcode>|   |1 bit: wide| |3 bits: register|
        */
        switch (byte >> 4) {
            case 0b00001011:
                bool error = handle_immediate_mov(byte, infile, outfile);
                if (error) {
                    return 1;
                }
                goto loop;
            default:
                break;
        }

        /*  Opcode Layout:
            _  _  _  _  _  _     _                    _
            |6 bits : opcode>|   |1 bit: destination| |1 bit: wide|
        */
        switch (byte >> 2) {
        case 0b00100010: /* FALLTHROUGH */
        case 0b00110001: /* FALLTHROUGH */
        case 0b00101000: /* FALLTHROUGH */
        case 0b00100011:
            bool error = handle_mov(byte, infile, outfile);
            if (error) {
                return 1;
            }
            goto loop;
        default:
            fprintf(stderr, "error: unknown opcode: %x\n", byte>>2);
            break;
        }

    }
    return 0;
}
