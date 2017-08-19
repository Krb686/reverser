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
#define OP_FMT_R	2
#define OP_FMT_A	3
#define OP_FMT_N	4


// Processor modes
#define CPU_MODE_REAL		1
#define CPU_MODE_PROT		2
#define CPU_MODE_64_COMPAT	3
#define CPU_MODE_64_NORMAL	4
#define CPU_MODE_SMM		5

// Register options
#define REG_OPTS_0 "AL", "AX", "EAX", "MM0", "XMM0"
#define REG_OPTS_1 "CL", "CX", "ECX", "MM1", "XMM1"
#define REG_OPTS_2 "DL", "DX", "EDX", "MM2", "XMM2"
#define REG_OPTS_3 "BL", "BX", "EBX", "MM3", "XMM3"
#define REG_OPTS_4 "AH", "SP", "ESP", "MM4", "XMM4"
#define REG_OPTS_5 "CH", "BP", "EBP", "MM5", "XMM5"
#define REG_OPTS_6 "DH", "SI", "ESI", "MM6", "XMM6"
#define REG_OPTS_7 "BH", "DI", "EDI", "MM7", "XMM7"
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
#define ADDRESS_SZ_16	1
#define ADDRESS_SZ_32	2
#define ADDRESS_SZ_64	3

// Operand sizes
#define OPERAND_SZ_16	1
#define OPERAND_SZ_32	2


void decode_instructions(unsigned char*, int, uint8_t);
void change_state(int);
#endif
