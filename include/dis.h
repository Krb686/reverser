#ifndef DIS_H
#define DIS_H
#include <inttypes.h>

#define OPCODE       1
#define NOP          2
#define DISPLACEMENT 3
#define MODRM        4
#define SIB          5
#define OPERAND      6

// Operand format
#define OP_FMT_RR	0
#define OP_FMT_IR	1
#define OP_FMT_Rs	2
#define OP_FMT_Rd	3
#define OP_FMT_A	4
#define OP_FMT_N	5


// Processor modes
#define CPU_MODE_REAL		0
#define CPU_MODE_PROT		1
#define CPU_MODE_64_COMPAT	2
#define CPU_MODE_64_NORMAL	3
#define CPU_MODE_SMM		4

#define REG_OPTS_LINEUP "ROPT", "ROPT", "ROPT", "ROPT", "ROPT", "ROPT", "ROPT", "ROPT"




// Effective Addresses - 16-bit addressing mode (vol 2A 2-5)
// - This defines the effective address arrays for 16-bit addressing mode based on mode field of the ModRM byte.  The final value is selected by the RM field.
#define EADDR_16_MODE0 "[BX+SI]",        "[BX+DI]",        "[BP+SI]",        "[BP+DI]",        "[SI]",        "[DI]",        "disp16",      "[BX]"
#define EADDR_16_MODE1 "[BX+SI]+disp8",  "[BX+DI]+disp8",  "[BP+SI]+disp8",  "[BP+DI]+disp8",  "[SI]+disp8",  "[DI]+disp8",  "[BP]+disp8",  "[BX]+disp8"
#define EADDR_16_MODE2 "[BX+SI]+disp16", "[BX+DI]+disp16", "[BP+SI]+disp16", "[BP+DI]+disp16", "[SI]+disp16", "[DI]+disp16", "[BP]+disp16", "[BX]+disp16"
#define EADDR_16_MODE3 REG_OPTS_LINEUP

// Effective Addresses - 32-bit addressing mode (vol 2A 2-5)
// - This defines the effective address arrays for 16-bit addressing mode based on mode field of the ModRM byte.  The final value is selected by the RM field.
#define EADDR_32_MODE0 "[EAX]",        "[ECX]",        "[EDX]",        "[EBX]",        "SIB",        "disp32",       "[ESI]",        "[EDI]"
#define EADDR_32_MODE1 "[EAX]+disp8",  "[ECX]+disp8",  "[EDX]+disp8",  "[EBX]+disp8",  "SIB+disp8",  "[EBP]+disp8",  "[ESI]+disp8",  "[EDI]+disp8"
#define EADDR_32_MODE2 "[EAX]+disp32", "[ECX]+disp32", "[EDX]+disp32", "[EBX]+disp32", "SIB+disp32", "[EBP]+disp32", "[ESI]+disp32", "[EDI]+disp32"
#define EADDR_32_MODE3 REG_OPTS_LINEUP


// Address sizes
#define ADDRESS_SZ_16	0
#define ADDRESS_SZ_32	1
#define ADDRESS_SZ_64	2

// Operand sizes
#define OPERAND_SZ_16	1
#define OPERAND_SZ_32	2

// Addressing Modes (Vol. 2D A-1)      //      field                   // selects
#define ADDR_MODE_A            0       //      direct address
#define ADDR_MODE_B            1       //      VEX.vvvv                GPR
#define ADDR_MODE_C            2       //      ModR/M reg              control register
#define ADDR_MODE_D            3       //      ModR/M reg              debug register
#define ADDR_MODE_E            4       //      ModR/M                  GPR/memory
#define ADDR_MODE_F            5       //      -                       EFLAGS/RFLAGS
#define ADDR_MODE_G            6       //      ModR/M reg              GPR
#define ADDR_MODE_H            7       //      VEX.vvvv                XMM/YMM
#define ADDR_MODE_I            8       //      -                       immediate
#define ADDR_MODE_J            9       //      -                       relative offset
#define ADDR_MODE_K            10      // ---- UNUSED -------------------------------------------
#define ADDR_MODE_L            11      //      top 4 of 8-bit imm      XMM/YMM
#define ADDR_MODE_M            12      //      ModR/M                  memory
#define ADDR_MODE_N            13      //      ModR/M r/m              MMX (packed quad)
#define ADDR_MODE_O            14      //      -                       offset (word/double word)
#define ADDR_MODE_P            15      //      ModR/M reg              MMX (packed quad)
#define ADDR_MODE_Q            16      //      ModR/M                  MMX/memory
#define ADDR_MODE_R            17      //      ModR/M r/m              GPR
#define ADDR_MODE_S            18      //      ModR/M reg              segment register
#define ADDR_MODE_T            19      // ---- UNUSED ------------------------------------------
#define ADDR_MODE_U            20      //      ModR/M r/m              XMM/YMM
#define ADDR_MODE_V            21      //      ModR/M reg              XMM/YMM
#define ADDR_MODE_W            22      //      ModR/M                  XMM/YMM/memory
#define ADDR_MODE_X            23      //      DS:rSI                  memory
#define ADDR_MODE_Y            24      //      ES:rDI                  memory
// These are 'pseudomodes' defined to capture the possible register codes (A.2.3)
#define ADDR_MODE_REGCODE_1    25      //      -                       exact register (AX)
#define ADDR_MODE_REGCODE_2    26      //      -                       exact register (eXX    - AL/AX)
#define ADDR_MODE_REGCODE_3    27      //      -                       exact register (rXX    - AL/AX/RAX)
#define ADDR_MODE_REGCODE_4    28      //      -                       exact register (rxx/r# - AL/AX/RAX/r9)
#define ADDR_MODE_NONE         99


struct rextype {
    uint8_t w;
    uint8_t r;
    uint8_t x;
    uint8_t b;
};

struct state_core {
    uint8_t __state;
    uint8_t __state_next;
    struct rextype rex;
};

void decode_instructions(unsigned char*, int, uint8_t);
void change_state(int, struct state_core*);
#endif
