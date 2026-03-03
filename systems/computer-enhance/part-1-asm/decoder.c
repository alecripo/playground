#include <errno.h>
#include<stdio.h>
#include<stdint.h>
#include <string.h>
#include<stdbool.h>

int main(int argc, char** argv) {
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

    char* registers_small[8] = {"AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH"};
    char* registers_wide[8] = {"AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI"};
    char** registers_src;
    char* displacements[8] = {"BX+SI", "BX+DI", "BP+SI", "BP+DI", "SI", "DI", "", "BX"};
    int byte;
    while (true) {
        byte = fgetc(infile);
        if (byte == EOF) {
            break;
        }
        // Layout: _  _  _  _  _  _     _             _
        //        |6 bits : opcode>|   |1 bit: dest| |1 bit: wide|
        bool dest = byte & 0b00000010;
        bool wide = byte & 0b00000001;
        switch (byte >> 2) {
            // MOV
            case 0x100010:
            case 0x110001:
            case 0x101000:
            case 0x100011:
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
                if (mode == 0x00) {

                } else if (mode == 0x01 || mode == 0x02) {

                } else {
                    if (wide) {
                        registers_src = registers_wide;
                    } else {
                        registers_src = registers_small;
                    }
                    char* dest = registers_src[reg];
                    char* src = registers_src[reg_or_mem];
                    if (!dest) {
                        char* aux = dest;
                        dest = src;
                        src=aux;
                    }
                    fprintf(outfile, "MOV %s, %s", dest, src);
                }
                break;
            default:
                fprintf(stderr, "error: unknown byte: %x\n", byte);
                break;
        }
    }
    return 0;
}
