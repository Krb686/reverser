#ifndef PRINT_H
#define PRINT_H

#define DEBUG

#ifdef DEBUG
  #define PRINTD printf
#else
  #define PRINTD(format, args...) ((void)0)
#endif

#include <inttypes.h>
#include "dis.h"
void print_bytes(char*, int);
//void print_instruction(const int, const char*, const char*, const char*);
void print_instruction(struct state_core*);
#endif
