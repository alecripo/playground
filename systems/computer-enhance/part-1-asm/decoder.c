#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

char* REGISTERS_SMALL[8] = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};
char* REGISTERS_WIDE[8] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};
char* EFFECTIVE_ADDRESSES[8] = {"bx + si", "bx + di", "bp + si", "bp + di", "si", "di", "bp", "bx"};

typedef struct operands {
    char dest[32];
    char src[32];
} operands;

typedef enum parse_error {
    NO_ERROR = 0,
    EOF_ERROR = 1,
} parse_error;

typedef parse_error (op_handler)(int byte, FILE* infile, FILE* outfile);


parse_error
parse_displacement_op(int byte, FILE* instruction_stream, operands* oprs) {
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
    if ( (byte = fgetc(instruction_stream)) == EOF ) {
        fprintf(stderr, "Invalid byte stream: expected second byte of MOV, got EOF");
        return EOF_ERROR;
    }
    uint8_t mode = (byte >> 6);
    uint8_t reg = (byte & 0b00111000) >> 3;
    uint8_t reg_or_mem = (byte & 0b00000111);

    strncpy(oprs->dest, register_names[reg], 32);
    strncpy(oprs->src, register_names[reg_or_mem], 32);

    switch (mode) {
    case 0x00:
        if (reg_or_mem != 0b00000110) {
            sprintf(oprs->src, "[%s]", EFFECTIVE_ADDRESSES[reg_or_mem]);
            break;
        }
        int16_t imm_val = 0;
        byte = fgetc(instruction_stream);
        if (byte == EOF) {
            fprintf(stderr, "Invalid byte stream: expected immediate value, got EOF");
            return EOF_ERROR;
        }
        imm_val |= byte;
        byte = fgetc(instruction_stream);
        if (byte == EOF) {
            fprintf(stderr, "Invalid byte stream: expected immediate value, got EOF");
            return EOF_ERROR;
        }
        imm_val = ((int16_t)byte << 8) | imm_val;
        sprintf(oprs->src, "%d", imm_val);
        break;
    // handle 8-bit and 16-bit memory displacements in the same block
    case 0x01: /* FALLTHROUGH */
    case 0x02: {
        int16_t displacement = 0;
        if ((byte = fgetc(instruction_stream)) == EOF) {
            fprintf(stderr, "Invalid byte stream: expected 8-bit displacement, got EOF");
            return EOF_ERROR;
        }
        displacement |= byte;

        if (mode == 0x02) {
            byte = fgetc(instruction_stream);
            if (byte == EOF) {
                fprintf(stderr, "Invalid byte stream: expected 16-bit displacement, got EOF");
                return EOF_ERROR;
            }
            displacement |= ((int16_t)byte<<8);
        }

        if (displacement == 0) {
            sprintf(oprs->src, "[%s]", EFFECTIVE_ADDRESSES[reg_or_mem]);
        } else {
            sprintf(oprs->src, "[%s + %d]", EFFECTIVE_ADDRESSES[reg_or_mem], displacement);
        }
        break;
    }
    case 0x03: /* FALL THROUGH */
    default:
        break;
    }

    if (!is_first_op_dest) {
        char aux[32];
        strncpy(aux, oprs->dest, 32);
        strncpy(oprs->dest, oprs->src, 32);
        strncpy(oprs->src, aux, 32);
    }
    return NO_ERROR;
}

parse_error
parse_immediate_op(int byte, FILE* instruction_stream, operands* oprs) {
    bool is_wide = byte & 0b00001000;
    int16_t immediate=0;
    uint8_t reg = byte & 0b00000111;


    if ( (byte = fgetc(instruction_stream)) == EOF ) {
        fprintf(stderr, "Invalid byte stream: expected second byte of op, got EOF");
        return EOF_ERROR;
    }
    immediate |= byte;

    if (is_wide) {
        if ( (byte = fgetc(instruction_stream)) == EOF ) {
            fprintf(stderr, "Invalid byte stream: expected second byte of op, got EOF");
            return EOF_ERROR;
        }
        immediate = ((int16_t)byte)<<8 | immediate;
        strncpy(oprs->dest, REGISTERS_WIDE[reg], 32);
        snprintf(oprs->src, 32, "%d", immediate);
    }
    else {
        immediate = (immediate ^ 0x80)-0x80;
        strncpy(oprs->dest, REGISTERS_SMALL[reg], 32);
        snprintf(oprs->src, 32, "%d", immediate);
    }

    return NO_ERROR;
}

parse_error
handle_add(int byte, FILE* infile, FILE* outfile) {
    operands oprs;
    parse_error err = parse_displacement_op(byte, infile, &oprs);
    if (err != NO_ERROR) {
        return err;
    }
    fprintf(outfile, "add %s, %s\n", oprs.dest, oprs.src);
    return NO_ERROR;
}

parse_error
handle_mov(int byte, FILE* infile, FILE* outfile) {
    operands oprs;
    parse_error err = parse_displacement_op(byte, infile, &oprs);
    if (err != NO_ERROR) {
       return err;
    }
    fprintf(outfile, "mov %s, %s\n", oprs.dest, oprs.src);
    return NO_ERROR;
}

parse_error
handle_immediate_mov(int op_byte, FILE* infile, FILE* outfile) {
    operands oprs;
    parse_error err = parse_immediate_op(op_byte, infile, &oprs);
    if (err != NO_ERROR) {
        return err;
    }
    fprintf(outfile, "mov %s, %s\n", oprs.dest, oprs.src);
    return NO_ERROR;
}

parse_error
handle_immediate_add(int op_byte, FILE* infile, FILE* outfile) {
    operands oprs;
    parse_error err = parse_immediate_op(op_byte, infile, &oprs);
    if (err != NO_ERROR) {
        return err;
    }
    fprintf(outfile, "add %s, %s\n", oprs.dest, oprs.src);
    return NO_ERROR;
}

parse_error not_implemented(int byte, FILE* infile, FILE* outfile) {
    fprintf(stderr, "error: unknown opcode: %x\n", byte);
    return NO_ERROR;
}

int
main(int argc, char** argv) {
    if (argc != 3) {
        printf("Usage: decoder <IN_FILE> <OUT_FILE>\n\
            <IN_FILE>: binary file that will be decoded to 8086 assembly\n\
            <OUTFILE>: file where output will be written. '-' writes to stdout\n");
        return 1;
    }
    FILE* infile = fopen(argv[1], "rb");
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

    uint8_t i = 0;
    op_handler *op_handlers[256];
    for (i=0x0; i<0xff; i++) {
        op_handlers[i] = not_implemented;
    }
    for (i=0x00; i<0x04; i++) {
        op_handlers[i] = handle_add;
    }
    op_handlers[0x04] = handle_immediate_add;
    op_handlers[0x05] = handle_immediate_add;
    op_handlers[0x88] = handle_mov;
    op_handlers[0x89] = handle_mov;
    op_handlers[0x8a] = handle_mov;
    op_handlers[0x8b] = handle_mov;
    op_handlers[0x8c] = handle_mov;
    op_handlers[0x8e] = handle_mov;
    op_handlers[0xc6] = handle_mov;
    op_handlers[0xc7] = handle_mov;
    for (i=0xb0; i<0xc0; i++) {
        op_handlers[i]=handle_immediate_mov;
    }

    int op_first_byte;
    loop:
    while (true) {
        op_first_byte = fgetc(infile);
        if (op_first_byte == EOF) {
            break;
        }
        int err = op_handlers[(uint8_t)op_first_byte](op_first_byte, infile, outfile);
        if (err) {
            break;
        }
    }
    return 0;
}
