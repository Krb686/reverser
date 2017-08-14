#ifndef DIS_H
#define DIS_H

#define OPCODE       1
#define NOP          2
#define DISPLACEMENT 3
#define MODRM        4
#define SIB          5
#define OPERAND      6

void decode_instructions(unsigned char*, int);
void change_state(int);
#endif
