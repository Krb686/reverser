#ifndef STR_H
#define STR_H
#include <inttypes.h>

#define UNSIGNED 0
#define SIGNED   1

void reverse_str(char*);
char* trim_leading(char*, char);
uint8_t is_hex(char*);
char* sign_extend(char*, uint8_t);
char* hex2bin(char*);
char* bin2hex(char*, uint8_t);
#endif
