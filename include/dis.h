#ifndef DIS_H
#define DIS_H

#define OPCODE       1
#define NOP          2
#define DISPLACEMENT 3
#define MODRM        4
#define SIB          5
#define OPERAND      6


#define OP_FMT_RR	0
#define OP_FMT_IR	1
#define OP_FMT_R	2
#define OP_FMT_A	3
#define OP_FMT_N	4


void decode_instructions(unsigned char*, int);
void change_state(int);
#endif
