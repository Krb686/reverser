#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dis.h"
#include "print.h"
#include "str.h"


//printf("(REX: %s)", REX_STRS[*prefix_rex]);
//


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

struct instr {
    char *name;
    char *bytes;
    struct section *sptr;
    uint64_t offset;
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
        9, 9, 9, 9, 9, 9, 9, 9, OP_FMT_IR, 9, 9, 9, 9, 9, 9, 9, \
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

int __state_next  = OPCODE;

void decode_instructions(unsigned char *byte, int numbytes){

    int OPERAND_BYTES[8] = { 0, 0, 0, 0, 0, 0, 0, 0};

    __bytenum = 0;
    char byte_str[3];

    uint8_t opcode_sz = 1;

    const char *instr_name;
    const char *op_src;
    const char *op_dst;
    //char *op_dst;
    char op_str[16];
    //char *op_str = malloc(sizeof(char)*32);
    op_str[0] = '\0';

    int groupnum = 0;

    uint8_t modefield = 0;
    uint8_t regfield = 0;
    uint8_t rmfield = 0;

    uint8_t prefix_rex = 0;

    // TODO - make this into more of a state machine
    int __state       = OPCODE;

    //int flag_nop = 0;
    int flag_skip_opcode = 0;
    int num_operands = 0;
    int op_bytes_index = 0;
    int flag_displacement = 0;
    int flag_sib = 0;
    int nop_len = 0;
    int opcode_reg = 0;
    int operand_format = 0;

    const char *reg;

    // Loop over all bytes
    while(__bytenum < numbytes){
        printf("\t\t\t\tbyte = %x\n", *byte);
        switch(__state){
            case OPCODE:
                switch(*byte){
                case 0x01:
                    num_operands = 1;
                    OPERAND_BYTES[0] = 1;
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
                    OPERAND_BYTES[0] = 4;
                    break;
                case 0x31: //xor
                    break;
                case 0x41:
                    prefix_rex = *byte & 0xF;
                    break;
                case 0x44:
                    break;
                case 0x47:
                    prefix_rex = *byte & 0xF;
                    break;
                case 0x48:
                    prefix_rex = *byte & 0xF;
                    break;
                case 0x49:
                    prefix_rex = *byte & 0xF;
                    break;
                case 0x4c:
                    prefix_rex = *byte & 0xF;
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
                    OPERAND_BYTES[0] = 1;
                    break;
                case 0x75:
                    num_operands = 1;
                    OPERAND_BYTES[0] = 1;
                    break;
                case 0x77:
                    num_operands = 1;
                    OPERAND_BYTES[0] = 1;
                    break;
                case 0x80:
                    groupnum = 1;
                    OPERAND_BYTES[0] = 1;
                    break;
                case 0x83:
                    printf("\t\t\t\tgroup 1\n");
                    groupnum = 1;
                    // Ev, Ib (1 modrm byte, 1 imm byte)
                    OPERAND_BYTES[0] = 1;
                    break;
                case 0x85:
                    break;
                case 0x89:
                    break;
                case 0xba:
                    num_operands = 1;
                    OPERAND_BYTES[0] = 4;
                    break;
                case 0xb8:
                    // 1011wreg : imm
                    opcode_reg = 1;
                    __SELECT_W_PRESENT = 1;
                    __SELECT_W_VALUE = (*byte >> 3) & 0x1;
                    __SELECT_REG_VALUE = *byte & 0x7;
                    OPERAND_BYTES[0] = 4;
                    num_operands = 1;
                    break;
                case 0xbf:
                    num_operands = 1;
                    OPERAND_BYTES[0] = 4;
                    break;
                case 0x5d:
                    //mark_instr("pop");
                    break;
                case 0x5e:
                    //mark_instr("pop");
                    break;
                case 0xc1:
                    groupnum = 2;
                    num_operands = 1;
                    OPERAND_BYTES[0] = 1;
                    break;
                case 0xc3:
                    //mark_instr("retq");
                    break;
                case 0xc6:
                    //group 11, Eb, Ib
                    groupnum = 11;
                    num_operands = 1;
                    OPERAND_BYTES[0] = 1;
                    break;
                case 0xc7:
                    groupnum = 11;
                    printf("\t\t\t\tgroup 11\n");
                    // Ev, Iz (1 modrm byte, 1 immediate word (16 bit operand size) / double word (32 or 64 bit operand size)
                    OPERAND_BYTES[0] = 4;
                    break;
                case 0xd1:
                    groupnum = 2;
                    num_operands = 0;
                    break;
                case 0xe8:
                    num_operands = 1;
                    OPERAND_BYTES[0] = 4;
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

                // Determine the selected register
                if(opcode_reg){
                    reg = REG_ENCODING[__SELECT_PROC_MODE][__SELECT_W_PRESENT][__SELECT_W_VALUE][__SELECT_DATA_SIZE][__SELECT_REG_VALUE];
                    op_dst = reg;
                }

                operand_format = OPERAND_FORMATS[opcode_sz - 1][*byte];

                if(strcmp(reg, "") == 0){
                    printf("incorrect register decoding\n");
                    return;
                }
                PRINTD ("\t\t\t\tmode = %d\n", __SELECT_PROC_MODE);
                PRINTD ("\t\t\t\tw present = %d\n", __SELECT_W_PRESENT);
                PRINTD ("\t\t\t\tw value = %d\n", __SELECT_W_VALUE);
                PRINTD ("\t\t\t\tdata size = %d\n", __SELECT_DATA_SIZE);
                PRINTD ("\t\t\t\treg = %s\n", reg);

                // Skip assigning an instruction for the current opcode on special bytes (opcode size increase)
                if(!flag_skip_opcode){
                    instr_name = INSTR_NAMES[opcode_sz - 1][*byte];

                    if(instr_name != NULL && strcmp(instr_name, "") == 0){
                        //printf("instr not implemented!\n");
                        printf("unaccounted byte - exiting\n");
                        return;
                    } else {
                        //if(strcmp(instr_name, "REX") == 0 || strcmp(instr_name, "-") == 0){
                            // instruction can't be determined yet
                        //} else {
                        //    print_instruction(instr_name, op_src, "", &prefix_rex);
                        //}

                        change_state(STATE_NEXT_MAP[opcode_sz - 1][*byte]);
                    }
                }

                break;
            case DISPLACEMENT:
                PRINTD ("CASE --> DISPLACEMENT\n");
                if(DISPLACEMENT_BYTES > 0){
                    DISPLACEMENT_BYTES--;

                    if(DISPLACEMENT_BYTES == 0){
                        if(num_operands > 0){
                            change_state(OPERAND);
                        } else {
                            print_instruction(operand_format, instr_name, op_src, op_dst, &prefix_rex);
                            change_state(OPCODE);
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

                // displacement detection
                if( (modefield == 0 && rmfield == 5) || modefield == 1 || modefield == 2){
                    printf("found displacement\n");
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
                        instr_name = INSTR_NAMES_GROUP1[regfield];
                        if(instr_name != NULL && strcmp(instr_name, "") == 0){
                            //printf("instr not implemented!\n");
                        } else {
                            print_instruction(operand_format, instr_name, "", "", &prefix_rex);
                        }

                        num_operands = 1;
                        groupnum = 0;
                        break;
                    case 2:
                        instr_name = INSTR_NAMES_GROUP2[regfield];
                         if(instr_name != NULL && strcmp(instr_name, "") == 0){
                            printf("instr not implemented!\n");
                        } else {
                            print_instruction(operand_format, instr_name, "", "", &prefix_rex);
                        }
                        groupnum = 0;
                        break;
                    case 5:
                        instr_name = INSTR_NAMES_GROUP5[regfield];
                        if(instr_name != NULL && strcmp(instr_name, "") == 0){
                            printf("instr not implemented!\n");
                        } else {
                            print_instruction(operand_format, instr_name, "", "", &prefix_rex);
                        }
                        groupnum = 0;
                        break;
                    case 11:
                        instr_name = INSTR_NAMES_GROUP11[regfield];
                        // TODO - remove this duplicate code
                        if(instr_name != NULL && strcmp(instr_name, "") == 0){
                            printf("instr not implemented!\n");
                        } else {
                            print_instruction(operand_format, instr_name, "", "", &prefix_rex);
                        }

                        num_operands = 1;
                        groupnum = 0;
                        break;
                    }
                }

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
                    change_state(SIB);
                } else if(flag_displacement == 1){
                    change_state(DISPLACEMENT);
                } else if(num_operands > 0){
                    change_state(OPERAND);
                } else {
                    print_instruction(operand_format, instr_name, op_src, op_dst, &prefix_rex);
                    change_state(OPCODE);
                    opcode_sz = 1;
                }
                break;
            case NOP:
                PRINTD ("CASE --> NOP\n");
                if(nop_len > 0){
                    nop_len--;
                    if(nop_len == 0){
                        print_instruction(operand_format, instr_name, op_src, op_dst, &prefix_rex);
                        change_state(OPCODE);
                    }
                }
                break;
            case OPERAND:
                printf("CASE --> OPERAND\n");
                if(num_operands > 0){

                    if(OPERAND_BYTES[op_bytes_index] > 0){
                        OPERAND_BYTES[op_bytes_index]--;
                        snprintf(byte_str, 3, "%02x", *byte);
                        printf("byte_str = %s\n", byte_str);
                        strcat(op_str, byte_str);
                        printf("op_str = %s\n", op_str);
                    } else if(OPERAND_BYTES[op_bytes_index] == 0){ // Reduce operand count when operand bytes have been consumed
                        num_operands--;
                        op_bytes_index++;
                    }
                    

                    if(num_operands == 0){
                        
                        print_instruction(operand_format, instr_name, op_src, op_dst, &prefix_rex);
                        change_state(OPCODE);
                        op_bytes_index = 0;
                        opcode_sz = 1;
                        reverse_str(op_str);
                        printf("op_str = %s\n", op_str);
                        op_str[0] = '\0';
                    } else {
                        if(groupnum == 1){

                        }

                    }
                } else {
                    print_instruction(operand_format, instr_name, op_src, op_dst, &prefix_rex);
                    change_state(OPCODE);
                    op_bytes_index = 0;
                    opcode_sz = 1;
                    // Reset flags
                    groupnum = 0;
                }


                break;
            case SIB:
                PRINTD ("CASE --> SIB\n");
                if(flag_displacement){
                    change_state(DISPLACEMENT);
                } else {
                    print_instruction(operand_format, instr_name, op_src, op_dst, &prefix_rex);
                    change_state(OPCODE);
                    opcode_sz = 1;
                }
                flag_sib = 0;
                break;
        }


        byte++;
        __bytenum++;
        flag_skip_opcode = 0;
        // Assign the new state
        __state = __state_next;
    }
}

void change_state(int index){
    __state_next = index;
    printf("\t\t\t\tnext state --> %s\n", STATE_NEXT_STRINGS[index]);
}
