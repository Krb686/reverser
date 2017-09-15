#ifndef STR_H
#define STR_H
#include <inttypes.h>

#define UNSIGNED 0
#define SIGNED   1

void reverse_str(char*);
char* trim_leading(char*, char);
uint8_t is_hex(const char*);
char* sign_extend(const char*, uint8_t);
char* hex2bin(const char*);
char* bin2hex(char*, uint8_t);
#endif
