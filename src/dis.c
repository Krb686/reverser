// TODO
//  - make sure mapping indices can't go out of bounds
//  - figure out a better way to do mappings, maybe with pointers, especially the EADDR_32_MODE3
//  - rex.w promotion
//  *working - operand address mode maps
//  - need state info to determine when addressing mode can be referenced

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dis.h"
#include "print.h"
#include "str.h"

// 32-bit Addressing Forms with the ModR/M Byte
// index = [mod][rm]
const char *OP1EADDR[4][16] = 
{
    { EADDR_32_MODE0 },
    { EADDR_32_MODE1 },
    { EADDR_32_MODE2 },
    { EADDR_32_MODE3 }
};

// In 16-bit addressing mode, the selected register can be either 8 or 16 bits wide
// In 32-bit addressing mode, the selected register can be either 8 or 32 bits wide
// In 64-bit addressing mode, the selected register can be either 16, 32, or 64 bits wide
// The selected register is determined by the addressing mode, w-bit presence, and w-bit value
// For all addressing modes, register selection options when the w-bit is not present are the same as when the w-bit is present and w=1

// 3 addressing modes
// 2 w-bit values
// 8 reg encodings
const char *REG_OPTS_ARRAY[3][2][16] = {
    {
        { "AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH", "", "", "", "", "", "", "", "" },
        { "AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI", "", "", "", "", "", "", "", ""  }
    },
    {
        { "AL",  "CL",  "DL",  "BL",  "AH",  "CH",  "DH",  "BH", "", "", "", "", "", "", "", ""  },
        { "EAX", "ECX", "EDX", "EBX", "ESP", "EBP", "ESI", "EDI", "", "", "", "", "", "", "", ""  } 
    },
    {
        { "RAX", "RCX", "RDX", "RBX", "RSP", "RBP", "RSI", "RDI", "R8", "R9", "R10", "R11", "R12", "R13", "R14", "R15"  },
        { "RAX", "RCX", "RDX", "RBX", "RSP", "RBP", "RSI", "RDI", "R8", "R9", "R10", "R11", "R12", "R13", "R14", "R15"  }
    }
};
const char *REG_OPTS_ARRAY_64[8] = {"R8", "R9", "R10", "R11", "R12", "R13", "R14", "R15" };

// Default operand sizes for processor modes
//                                      real           prot           64-compat      64-norm        smm
const uint8_t DEFAULT_OPERAND_SZ[5] = { OPERAND_SZ_16, OPERAND_SZ_32, OPERAND_SZ_32, OPERAND_SZ_32, OPERAND_SZ_32 };
const uint8_t DEFAULT_ADDRESS_SZ[5] = { ADDRESS_SZ_16, ADDRESS_SZ_32, ADDRESS_SZ_32, ADDRESS_SZ_32, ADDRESS_SZ_32 };
const uint8_t OPERAND_SZ_BYTES[5] = { 2, 4, 4, 4, 4 };

const uint8_t OPERAND_SRC_ADDR_MODE;
const uint8_t OPERAND_DST_ADDR_MODE;

int __bytenum = 0;
int __opfound = 0;

int instr_count = 0;
int __SELECT_PROC_MODE = 0; //32 or 64 bit mode
int __SELECT_W_PRESENT = 0;
int __SELECT_DATA_SIZE = 1; //16, 32, or 64 bit
int __SELECT_W_VALUE = 0;
int __SELECT_REG_VALUE = 0;

struct symbol {
    char *name;
    uint8_t version;
    uint8_t type;
};


const char *INSTR_NAMES[3][256] =
{
    {
        "", "add", "", "", "", "", "", "",                          "", "", "", "", "", "", "", "-",    \
        "", "", "", "", "", "", "", "",                             "", "", "", "", "", "", "", "pop",  \
        "", "", "", "", "", "", "", "",                             "sub", "sub", "sub", "sub", "sub", "sub", "PREFIX_SEGCS", "",      \
        "", "xor", "", "", "", "", "", "",                          "cmp", "cmp", "cmp", "cmp", "cmp", "cmp", "", "",      \
        "REX", "REX", "REX", "REX", "REX", "REX", "REX",            "REX", "REX", "REX", "REX", "REX", "REX", "REX", "REX", "REX",        \
        "push", "", "", "push", "push", "push", "push", "push",     "pop", "pop", "pop", "pop", "pop", "pop", "pop", "pop",         \
        "", "", "", "", "", "", "-", "",                            "", "", "", "", "", "", "", "",         \
        "", "", "", "", "je", "jne", "", "ja",                      "", "", "", "", "", "", "", "",         \
        "-", "", "", "-", "", "test", "", "",                       "", "mov", "", "", "", "lea", "", "",         \
        "nop", "", "", "", "", "", "", "",                          "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "",                             "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "",                             "mov", "mov", "mov", "mov", "mov", "mov", "mov", "mov",         \
        "", "-", "", "ret", "", "", "-", "-",                       "", "", "", "", "", "", "", "",         \
        "", "-", "", "", "", "", "", "",                            "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "",                             "call", "jmp", "", "", "", "", "", "",         \
        "", "", "", "-", "hlt", "", "", "",                         "", "", "", "", "", "", "", "-"
    },
    {
        "", "", "", "", "", "", "", "",     "", "", "", "", "", "", "", "",    \
        "", "", "", "", "", "", "", "",     "", "", "", "", "", "", "", "nop",         \
        "", "", "", "", "", "", "", "",     "", "", "", "", "", "", "", "",      \
        "", "", "", "", "", "", "", "",     "", "", "", "", "", "", "", "",      \
        "", "", "", "", "", "", "", "",     "", "", "", "", "", "", "", "",        \
        "", "", "", "", "", "", "", "",     "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "",     "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "",     "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "",     "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "",     "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "",     "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "",     "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "",     "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "",     "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "",     "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "",     "", "", "", "", "", "", "", ""
    },
    {
        "", "", "", "", "", "", "", "",     "", "", "", "", "", "", "", "",    \
        "", "", "", "", "", "", "", "",     "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "",     "", "", "", "", "", "", "", "",      \
        "", "", "", "", "", "", "", "",     "", "", "", "", "", "", "", "",      \
        "", "", "", "", "", "", "", "",     "", "", "", "", "", "", "", "",        \
        "", "", "", "", "", "", "", "",     "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "",     "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "",     "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "",     "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "",     "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "",     "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "",     "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "",     "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "",     "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "",     "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "",     "", "", "", "", "", "", "", ""
    }
};


const int STATE_NEXT_MAP[3][256] =
{
    {
        9, MODRM, 9, 9, 9, 9, 9, 9,                                         9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9,                                             9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9,                                             9, MODRM, 9, 9, 9, OPERAND, OPCODE, 9, \
        9, MODRM, 9, 9, 9, 9, 9, 9,                                         MODRM, MODRM, MODRM, MODRM, MODRM, MODRM, 9, 9, \
        OPCODE, OPCODE, OPCODE, OPCODE, OPCODE, OPCODE, OPCODE, OPCODE,     OPCODE, OPCODE, OPCODE, OPCODE, OPCODE, OPCODE, OPCODE, OPCODE, \
        OPCODE, 9, 9, OPCODE, OPCODE, OPCODE, OPCODE, OPCODE,               OPCODE, OPCODE, OPCODE, OPCODE, OPCODE, OPCODE, OPCODE, OPCODE, \
        9, 9, 9, 9, 9, 9, OPCODE, 9,                                        9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, OPERAND, OPERAND, 9, OPERAND,                           9, 9, 9, 9, 9, 9, 9, 9, \
        MODRM, 9, 9, MODRM, 9, MODRM, 9, 9,                                 9, MODRM, 9, 9, 9, MODRM, 9, 9, \
        OPCODE, 9, 9, 9, 9, 9, 9, 9,                                        9, MODRM, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9,                                             9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9,                                             OPERAND, 9, OPERAND, 9, 9, 9, 9, OPERAND, \
        9, MODRM, 9, OPCODE, 9, 9, MODRM, MODRM,                            9, 9, 9, 9, 9, 9, 9, 9, \
        9, MODRM, 9, 9, 9, 9, 9, 9,                                         9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9,                                             OPERAND, DISPLACEMENT, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, OPCODE, OPCODE, 9, 9, 9,                                   9, 9, 9, 9, 9, 9, 9, MODRM
    },
    {
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, MODRM, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9
    },
    {
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9
    }
};

const int OPERAND_FORMATS[3][256] = {
    {
        9, 9, 9, 9, 9, 9, 9, 9,     9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9,     9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9,     9, 9, 9, 9, 9, 9, 9, 9, \
        9, OP_FMT_RR, 9, 9, 9, 9    , 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9,     OP_FMT_Rs, OP_FMT_Rs, 9, 9, 9, 9, 9, 9, \
        OP_FMT_Rs, OP_FMT_Rs, OP_FMT_Rs, OP_FMT_Rs, OP_FMT_Rs, OP_FMT_Rs, OP_FMT_Rs, OP_FMT_Rs,     OP_FMT_Rd, OP_FMT_Rd, OP_FMT_Rd, OP_FMT_Rd, OP_FMT_Rd, OP_FMT_Rd, OP_FMT_Rd, OP_FMT_Rd, \
        9, 9, 9, 9, 9, 9, 9, 9,     9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9,     9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, OP_FMT_IR, 9, 9, 9, 9,     9, OP_FMT_RR, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9,     9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9,     9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9,     OP_FMT_IR, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, OP_FMT_N, 9, 9,    9, OP_FMT_IR, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9,     9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9,     OP_FMT_A, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9,     9, 9, 9, 9, 9, 9, 9, 9
    },
    {
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9
    },
    {
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9
    }
};

const int ADDR_MODES_SRC[3][256] = {
    {
        9, 9, 9, 9, 9, 9, 9, 9,     9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9,     9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9,     9, 9, 9, 9, 9, 9, 9, 9, \
        9, ADDR_MODE_G, 9, 9, 9, 9, 9, 9,     9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9,     9, 9, 9, 9, 9, 9, 9, 9, \
        ADDR_MODE_REGCODE_4, 9, 9, 9, ADDR_MODE_REGCODE_4, 9, 9, 9,     9, 9, 9, 9, 9, 9, ADDR_MODE_REGCODE_4, 9, \
        9, 9, 9, 9, 9, 9, 9, 9,     9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9,     9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, ADDR_MODE_I, 9, 9, 9, 9,     9, ADDR_MODE_G, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9,     9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9,     9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9,     ADDR_MODE_I, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, ADDR_MODE_I,     9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9,     9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9,     9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9,     9, 9, 9, 9, 9, 9, 9, 9
    },
    {
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9
    },
    {
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9
    }
};

const int ADDR_MODES_DST[3][256] = {
    {
        9, 9, 9, 9, 9, 9, 9, 9,     9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9,     9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9,     9, 9, 9, 9, 9, 9, 9, 9, \
        9, ADDR_MODE_E, 9, 9, 9, 9, 9, 9,     9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9,     9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9,     9, 9, 9, 9, 9, 9, ADDR_MODE_REGCODE_4, 9, \
        9, 9, 9, 9, 9, 9, 9, 9,     9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9,     9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, ADDR_MODE_E, 9, 9, 9, 9,     9, ADDR_MODE_E, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9,     9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9,     9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9,     ADDR_MODE_REGCODE_4, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, ADDR_MODE_E,     9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9,     9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9,     ADDR_MODE_J, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9,     9, 9, 9, 9, 9, 9, 9, 9
    },
    {
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9
    },
    {
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9
    }
};

//const uint8_t OPERAND_ADDRESS_MODES = 






//  32        | 64 bit mode
//  //  w not present | w present
//  //  w = 0 | w = 1
//  //  16 | 32 | 64 bit data
//  //  values
//

const char *REG_ENCODING[2][2][2][3][8] = {
                                              { // non-64 bit mode
                                                  { //w field not present
                                                      { // w = 0
                                                          { "ax",  "cx",  "dx",  "bx",  "sp",  "bp",  "si",  "di"},
                                                          { "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi" },
                                                          { "",    "",    "",    "",    "",    "",    "",    "" }
                                                      },
                                                      { //w = 1
                                                          { "", "", "", "", "", "", "", ""},
                                                          { "", "", "", "", "", "", "", "" },
                                                          { "", "", "", "", "", "", "", "" }
                                                      }
                                                  },
                                                  { // w field present
                                                      { //w = 0
                                                          { "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh" }, //16 bit data
                                                          { "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh" }, //32 bit data
                                                          { "",   "",   "",   "",   "",   "",   "",   ""   }  //invalid
                                                      },
                                                      { //w = 1
                                                          { "ax", "cx", "dx", "bx", "sp", "bp", "si", "di" }, //16 bit data
                                                          { "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi" },
                                                          { "", "", "", "", "", "", "", "" }
                                                      }
                                                  }
                                              },
                                              { // 64 bit mode
                                                  { // w field not present
                                                      { // w = 0
                                                          { "ax",  "cx",  "dx",  "bx",  "sp",  "bp",  "si",  "di"},
                                                          { "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi" },
                                                          { "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi" }
                                                      },
                                                      { // w = 1
                                                          { "", "", "", "", "", "", "", ""},
                                                          { "", "", "", "", "", "", "", "" },
                                                          { "", "", "", "", "", "", "", "" }
                                                      }
                                                  },
                                                  { // w field present
                                                      { // w = 0
                                                          { "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh" },
                                                          { "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh" },
                                                          { "", "", "", "", "", "", "", "" }
                                                      },
                                                      { // w = 1
                                                          { "ax", "cx", "dx", "bx", "sp", "bp", "si", "di"},
                                                          { "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi" },
                                                          { "", "", "", "", "", "", "", "" }
                                                      }
                                                  }
                                              }
                                          };




const char *INSTR_NAMES_GROUP1[8] = { "add", "or", "adc", "sbb", "and", "sub", "xor", "cmp" };
const char *INSTR_NAMES_GROUP2[8] = { "rol", "ror", "rcl", "rcr", "shl", "shr", "", "sar" };
const char *INSTR_NAMES_GROUP5[8] = { "inc", "dec", "call", "call", "jmp", "jmp", "push", "" };
const char *INSTR_NAMES_GROUP11[8] = { "mov", "",   "",    "",    "",    "",    "",    "xbegin" };
const char *REX_STRS[16] = { "----", "---B", "--X-", "--XB", "-R--", "-R-B", "-RX-", "-RXB", "W---", "W--B", "W-X-", "W-XB", "WR--", "WR-B", "WRX-", "WRXB", };


int DISPLACEMENT_BYTES = 0;
const char *STATE_NEXT_STRINGS[7] = { "", "OPCODE", "", "", "MODRM", "SIB", "OPERAND" };

void decode_instructions(unsigned char *byte, int numbytes, uint8_t elfclass, uint64_t startaddr){

    // Core of the state machine
    struct state_core sc = {
        .__state = OPCODE,
        .__state_next = OPCODE,
        .operand_fmt = OP_FMT_RR,
        .rex = {
            .w = 0,
            .r = 0,
            .x = 0,
            .b = 0
        },
        .cur_addr = startaddr,
        .instr_bytes[0] = '\0'
    };


    // Set processor mode based on ELFCLASS
    uint8_t cpumode;
    if (elfclass == 1){
        cpumode = CPU_MODE_PROT;
    } else if(elfclass == 2){
        cpumode = CPU_MODE_64_NORMAL;
    }
    PRINTD ("\tcpumode = %d\n", cpumode);

    // Set the default operand size accoding to the processor mode.
    uint8_t operand_sz = DEFAULT_OPERAND_SZ[cpumode];
    PRINTD ("\tdefault operand sz = %d\n", operand_sz);
    // Set the bytes per operand according to default operand size.
    uint8_t operand_bytes = OPERAND_SZ_BYTES[operand_sz];
    PRINTD ("\tbytes per operand = %d\n", operand_bytes);

    // Set the default address size according to the processor mode.
    uint8_t default_addr_sz = DEFAULT_ADDRESS_SZ[cpumode];

    int OPERAND_LENS[8] = { 0, 0, 0, 0, 0, 0, 0, 0};

    __bytenum = 0;
    char byte_str[3];

    uint8_t opcode_sz = 1;

    uint8_t w_bit = 1; 

    char op_str[16];
    op_str[0] = '\0';

    int groupnum = 0;

    uint8_t modefield = 0;
    uint8_t regfield = 0;
    uint8_t rmfield = 0;

    //int flag_nop = 0;
    int flag_skip_opcode = 0;
    int num_operands = 0;
    int op_bytes_index = 0;
    int flag_displacement = 0;
    int flag_sib = 0;
    int nop_len = 0;

    // Addressing modes selected from ADDR_MODE_SRC and ADDR_MODE_DST
    uint8_t addr_mode_src = 0;
    uint8_t addr_mode_dst = 0;

    // Loop over all bytes
    while(__bytenum < numbytes){
        char bytestr[3];
        snprintf(bytestr, 3, "%02x", *byte);
        PRINTD ("\t\t\t\tbyte = %s\n", bytestr);
        strcat(sc.instr_bytes, bytestr);
        switch(sc.__state){
            case OPCODE:
                switch(*byte){
                case 0x01:
                    num_operands = 1;
                    OPERAND_LENS[0] = 1;
                    break;
                case 0x0f:
                    if(opcode_sz == 1){
                        opcode_sz++;
                        flag_skip_opcode = 1;
                    }
                    break;
                case 0x1f:
                   if(opcode_sz == 2){
                       //flag_nop = 1;
                   }
                   break;
                case 0x29:
                    num_operands = 0;
                    break;
                case 0x2d:
                    num_operands = 1;
                    OPERAND_LENS[0] = 4;
                    break;
                case 0x31: //xor
                    break;
                case 0x41:
                    default_addr_sz = ADDRESS_SZ_64;
                    break;
                case 0x44:
                    default_addr_sz = ADDRESS_SZ_64;
                    break;
                case 0x47:
                    default_addr_sz = ADDRESS_SZ_64;
                    break;
                case 0x48:
                    default_addr_sz = ADDRESS_SZ_64;
                    break;
                case 0x49:
                    default_addr_sz = ADDRESS_SZ_64;
                    sc.rex.w = 1;
                    sc.rex.b = 1;
                    PRINTD ("set rex.b to 1\n");
                    break;
                case 0x4c:
                    default_addr_sz = ADDRESS_SZ_64;
                    break;
                case 0x50:
                    //mark_instr("push");
                    break;
                case 0x55:
                    //mark_instr("push");
                    break;
                case 0x66:
                    break;
                case 0x74:
                    num_operands = 1;
                    OPERAND_LENS[0] = 1;
                    break;
                case 0x75:
                    num_operands = 1;
                    OPERAND_LENS[0] = 1;
                    break;
                case 0x77:
                    num_operands = 1;
                    OPERAND_LENS[0] = 1;
                    break;
                case 0x80:
                    groupnum = 1;
                    OPERAND_LENS[0] = 1;
                    break;
                case 0x83:
                    printf("\t\t\t\tgroup 1\n");
                    groupnum = 1;
                    // Ev, Ib (1 modrm byte, 1 imm byte)
                    OPERAND_LENS[0] = 1;
                    break;
                case 0x85:
                    break;
                case 0x89:
                    break;
                case 0xba:
                    num_operands = 1;
                    OPERAND_LENS[0] = 4;
                    break;
                case 0xb8:
                    // 1011wreg : imm
                    __SELECT_W_PRESENT = 1;
                    __SELECT_W_VALUE = (*byte >> 3) & 0x1;
                    __SELECT_REG_VALUE = *byte & 0x7;
                    OPERAND_LENS[0] = 4;
                    num_operands = 1;
                    break;
                case 0xbf:
                    num_operands = 1;
                    OPERAND_LENS[0] = 4;
                    break;
                case 0xc1:
                    groupnum = 2;
                    num_operands = 1;
                    OPERAND_LENS[0] = 1;
                    break;
                case 0xc3:
                    //mark_instr("retq");
                    break;
                case 0xc6:
                    //group 11, Eb, Ib
                    groupnum = 11;
                    num_operands = 1;
                    OPERAND_LENS[0] = 1;
                    break;
                case 0xc7:
                    groupnum = 11;
                    printf("\t\t\t\tgroup 11\n");
                    // Ev, Iz (1 modrm byte, 1 immediate word (16 bit operand size) / double word (32 or 64 bit operand size)
                    OPERAND_LENS[0] = 4;
                    break;
                case 0xd1:
                    groupnum = 2;
                    num_operands = 0;
                    break;
                case 0xe8:
                    num_operands = 1;
                    OPERAND_LENS[0] = 4;
                    break;
                case 0xe9:
                    DISPLACEMENT_BYTES = 4;
                    printf("num_ops = %d\n", num_operands);
                    break;
                case 0xf4:
                    break;
                case 0xff:
                    groupnum = 5;
                    break;
                }

                // Assign the addressing modes
                addr_mode_src = ADDR_MODES_SRC[opcode_sz - 1][*byte];
                addr_mode_dst = ADDR_MODES_DST[opcode_sz - 1][*byte];


                if(addr_mode_src == ADDR_MODE_REGCODE_4){
                    regfield = (*byte) & 0x7;
                    // Check the operand width attribute
                    PRINTD ("default_addr_sz = %d\n", default_addr_sz);
                    PRINTD ("w_bit = %d\n", w_bit);
                    PRINTD ("regfield = %d\n", regfield);
                    sc.operand_src = REG_OPTS_ARRAY[default_addr_sz][w_bit][regfield];
                    PRINTD ("sc.operand_dst = %s\n", sc.operand_dst);
                } 

                // Determine the selected register
                // For 'implicit' register instructions, the lower 3 bits of the opcode specify which register
                if(addr_mode_dst == ADDR_MODE_REGCODE_4){
                    regfield = (*byte) & 0x7;
                    // Check the operand width attribute
                    PRINTD ("default_addr_sz = %d\n", default_addr_sz);
                    PRINTD ("w_bit = %d\n", w_bit);
                    PRINTD ("regfield = %d\n", regfield);
                    sc.operand_dst = REG_OPTS_ARRAY[default_addr_sz][w_bit][regfield];
                    PRINTD ("sc.operand_dst = %s\n", sc.operand_dst);
                }
                //
                // TODO - this needs to take into account the addressing mode
                //if(opcode_reg){
                //    reg = REG_ENCODING[__SELECT_PROC_MODE][__SELECT_W_PRESENT][__SELECT_W_VALUE][__SELECT_DATA_SIZE][__SELECT_REG_VALUE];
                //    sc.operand_dst = reg;
                //}


                //if(strcmp(reg, "") == 0){
                //    printf("incorrect register decoding\n");
                //    return;
                //}
                //PRINTD ("\t\t\t\tmode = %d\n", __SELECT_PROC_MODE);
                //PRINTD ("\t\t\t\tw present = %d\n", __SELECT_W_PRESENT);
                //PRINTD ("\t\t\t\tw value = %d\n", __SELECT_W_VALUE);
                //PRINTD ("\t\t\t\tdata size = %d\n", __SELECT_DATA_SIZE);
                //PRINTD ("\t\t\t\treg = %s\n", reg);

                // Skip assigning an instruction for the current opcode on special bytes (opcode size increase)
                if(!flag_skip_opcode){
                    sc.instr_name = INSTR_NAMES[opcode_sz - 1][*byte];

                    if(sc.instr_name != NULL && strcmp(sc.instr_name, "") == 0){
                        //printf("instr not implemented!\n");
                        
                        printf("unaccounted byte %x - exiting\n", *byte);
                        return;
                    } else {

                        int next_state = STATE_NEXT_MAP[opcode_sz - 1][*byte];
                        if(strcmp(sc.instr_name, "REX") == 0 || strcmp(sc.instr_name, "-") == 0){
                            // instruction can't be determined yet
                            PRINTD ("instruction not determined\n");
                            if(groupnum > 0){
                                sc.operand_fmt = OPERAND_FORMATS[opcode_sz - 1][*byte];
                                change_state(next_state, &sc);
                            }
                        } else {
                            PRINTD ("normal instruction\n");
                            sc.operand_fmt = OPERAND_FORMATS[opcode_sz - 1][*byte];
                            
                            if(num_operands == 0 && next_state == OPCODE){
                                add_instruction(&sc);
                                //print_instruction(&sc);
                            }
                            change_state(next_state, &sc);
                        }
                        
                    }
                }

                break;
            case DISPLACEMENT:
                PRINTD ("CASE --> DISPLACEMENT\n");
                if(DISPLACEMENT_BYTES > 0){
                    DISPLACEMENT_BYTES--;

                    if(DISPLACEMENT_BYTES == 0){
                        if(num_operands > 0){
                            change_state(OPERAND, &sc);
                        } else {
                            add_instruction(&sc);
                            //print_instruction(&sc);
                            change_state(OPCODE, &sc);
                            opcode_sz = 1;
                        }
                        flag_displacement = 0;
                    }
                }
                break;
            case MODRM:
                PRINTD ("CASE --> MODRM\n");
                modefield = (*byte >> 6) & 0x3;
                regfield = (*byte >> 3) & 0x7;
                rmfield = (*byte) & 0x7;

                // Effectively increase the size of these offsets based on the prepended rex bits
                if(sc.rex.r){
                    regfield = regfield+8;
                } 

                // If a ModRM byte is present, this modifies the r/m field.
                // If the opcode encodes a register, this modifies that register field
                if(sc.rex.b){
                    rmfield = rmfield+8;
                    PRINTD ("rex.b = 1\n");
                }

                // Assign the effective address 
                // - if the effective address is a REG_OPT string,
                //   we need to index into the correct REG_OPT array
                PRINTD ("about to assign operands\n");
                PRINTD ("modefield = %d\n", modefield);
                PRINTD ("regfield = %d\n", regfield);
                PRINTD ("rmfield = %d\n", rmfield);
                PRINTD ("default_addr_sz = %d\n", default_addr_sz);
                PRINTD ("w_bit = %d\n", w_bit);
                PRINTD ("rmfield = %d\n", rmfield);
                PRINTD ("addr_mode_dst = %d\n", addr_mode_dst);

                // WORKING - addressing mode checks
                switch(addr_mode_src) {
                    case ADDR_MODE_G:
                        sc.operand_src = REG_OPTS_ARRAY[default_addr_sz][w_bit][regfield];
                        break;
                }

                if(addr_mode_dst == ADDR_MODE_E){
                    sc.operand_dst = REG_OPTS_ARRAY[default_addr_sz][w_bit][rmfield];
                    PRINTD ("after assignment, sc.operand_dst = %s\n", sc.operand_dst);
                }

                if(modefield < 3){
                    sc.operand_src = OP1EADDR[modefield][rmfield];
                } //else {
                //    sc.operand_src = REG_OPTS_ARRAY[default_addr_sz][w_bit][regfield];
               // }

                 // TODO - always checking rmfield isn't correct. It's dependent on operand format
                 

                // displacement detection
                if( (modefield == 0 && rmfield == 5) || modefield == 1 || modefield == 2){
                    PRINTD ("found displacement\n");
                    flag_displacement = 1;
                    if(modefield == 0 || modefield == 2){
                        DISPLACEMENT_BYTES = 4;
                    } else{
                        DISPLACEMENT_BYTES = 1;
                    }
                }

                // SIB byte detection
                if(modefield <= 2 && rmfield == 4){
                    flag_sib = 1;
                }


                if(groupnum > 0){
                    switch(groupnum){

                    case 1:
                        sc.instr_name = INSTR_NAMES_GROUP1[regfield];
                        num_operands = 1;
                        groupnum = 0;
                        break;
                    case 2:
                        sc.instr_name = INSTR_NAMES_GROUP2[regfield];
                         if(sc.instr_name != NULL && strcmp(sc.instr_name, "") == 0){
                             PRINTD ("instr not implemented!\n");
                        } else {
                            add_instruction(&sc);
                            //print_instruction(&sc);
                        }
                        groupnum = 0;
                        break;
                    case 5:
                        sc.instr_name = INSTR_NAMES_GROUP5[regfield];
                        if(sc.instr_name != NULL && strcmp(sc.instr_name, "") == 0){
                            PRINTD ("instr not implemented!\n");
                        } else {
                            add_instruction(&sc);
                            //print_instruction(&sc);
                        }
                        groupnum = 0;
                        break;
                    case 11:
                        sc.instr_name = INSTR_NAMES_GROUP11[regfield];
                        num_operands = 1;
                        groupnum = 0;
                        break;
                    }
                }

                //if(modfield == 3){
                    //
                //}

                /*
                if(flag_nop){
                        switch(*byte){
                            case 0x00:
                                num_operands = 0;
                                break;
                            case 0x40:
                                OPERAND_BYTES[0] = 1;
                                break;
                            case 0x44:
                                OPERAND_BYTES[0] = 2;
                                break;
                            case 0x80:
                                OPERAND_BYTES[0] = 4;
                                break;
                            case 0x84:
                                OPERAND_BYTES[0] = 5;
                                break;
                        }
                        flag_nop = 0;
                    }
                */
                if(flag_sib == 1){
                    change_state(SIB, &sc);
                } else if(flag_displacement == 1){
                    change_state(DISPLACEMENT, &sc);
                } else if(num_operands > 0){
                    change_state(OPERAND, &sc);
                } else {
                    add_instruction(&sc);
                    //print_instruction(&sc);
                    change_state(OPCODE, &sc);
                    opcode_sz = 1;
                }
                break;
            case NOP:
                PRINTD ("CASE --> NOP\n");
                if(nop_len > 0){
                    nop_len--;
                    if(nop_len == 0){
                        add_instruction(&sc);
                        //print_instruction(&sc);
                        change_state(OPCODE, &sc);
                    }
                }
                break;
            case OPERAND:
                PRINTD ("CASE --> OPERAND\n");
                if(num_operands > 0){

                    // True when operands still have bytes left to read
                    if(OPERAND_LENS[op_bytes_index] > 0){
                        OPERAND_LENS[op_bytes_index]--;
                        snprintf(byte_str, 3, "%02x", *byte);
                        PRINTD ("byte_str = %s\n", byte_str);
                        strcat(op_str, byte_str);
                        PRINTD ("op_str = %s\n", op_str);
                    } 

                    // Reduce operand count when operand bytes have been consumed
                    if(OPERAND_LENS[op_bytes_index] == 0){ 
                        num_operands--;
                        op_bytes_index++;
                    }
                    
                    // When all operands have been read
                    if(num_operands == 0){
                        op_bytes_index = 0;
                        opcode_sz = 1;
                        reverse_str(op_str);
                        trim_leading(op_str, '0');
                        PRINTD ("op_str = %s\n", op_str);

                        // For immediate operands
                        if(addr_mode_src == ADDR_MODE_I ){
                            sc.operand_src = op_str;
                            if(sc.rex.w){
                                char *op_src_ext = sign_extend(sc.operand_src, 64);
                                sc.operand_src = op_src_ext;
                            }
                        }

                        // Relative offset to be added to the instruction pointer
                        if(addr_mode_dst == ADDR_MODE_J){
                            // Need to add (address of current instruction + len(current instr) + offset)
                            // By the time this runs, startaddr is instr addr + len(current instr)-1, so just add 1
                            sc.operand_dst = l2hex(sc.cur_addr + hex2l(op_str) + 1);
                        }

                        add_instruction(&sc);
                        //print_instruction(&sc);
                        change_state(OPCODE, &sc);
                        op_str[0] = '\0';
                    } else {
                        if(groupnum == 1){

                        }

                    }
                } else {
                    add_instruction(&sc);
                    //print_instruction(&sc);
                    change_state(OPCODE, &sc);
                    op_bytes_index = 0;
                    opcode_sz = 1;
                    // Reset flags
                    groupnum = 0;
                }


                break;
            case SIB:
                PRINTD ("CASE --> SIB\n");
                if(flag_displacement){
                    change_state(DISPLACEMENT, &sc);
                } else {
                    add_instruction(&sc);
                    //print_instruction(&sc);
                    change_state(OPCODE, &sc);
                    opcode_sz = 1;
                }
                flag_sib = 0;
                break;
        }


        byte++;
        sc.cur_addr++;
        __bytenum++;
        flag_skip_opcode = 0;
        // Assign the new state
        sc.__state = sc.__state_next;
    }

    printf("printing all instructions\n");
    print_instructions(&sc);
}

void change_state(int index, struct state_core *sc){
    sc->__state_next = index;

    PRINTD ("\t\t\t\tnext state --> %s\n", STATE_NEXT_STRINGS[index]);

    if(index == OPCODE){
        sc->rex.b = 0;
    }
}

void add_instruction(struct state_core *sc){
    PRINTD ("adding new instr\n");
    struct instr *i = malloc(sizeof(struct instr));

    // name
    i->name = strdup(sc->instr_name);
    PRINTD ("i->name = %s\n", i->name);

    // bytes
    i->bytes = strdup(sc->instr_bytes);
    PRINTD ("i->bytes = %s\n", i->bytes);
    sc->instr_bytes[0] = '\0';

    // src
    i->operand_src = strdup(sc->operand_src);
    PRINTD ("i->operand_src = %s\n", i->operand_src);
    // dst
    i->operand_dst = sc->operand_dst;

    i->operand_fmt = sc->operand_fmt;

    i->offset = sc->cur_addr;

    if(sc->instr_list == NULL){
        sc->instr_list =  i;
    } else {
        struct instr *last = sc->instr_list;
        while(last){
            if(last->n){
                last = last->n;
            } else{
                break;
            }
        } 
        last->n = i;
    }
    PRINTD ("done adding new instr\n");
}
