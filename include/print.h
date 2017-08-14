#ifndef PRINT_H
#define PRINT_H

#define DEBUG

#ifdef DEBUG
  #define PRINTD printf
#else
  #define PRINTD(format, args...) ((void)0)
#endif

#include "inttypes.h"
void print_bytes(char*, int);
void print_instruction(const char*, const char*, char*, uint8_t*);
#endif
