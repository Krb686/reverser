#include <stdio.h>

#include "print.h"

void print_bytes(char *byteptr, int n){
    int i;
    for(i=0;i<n;i++){
        printf("%02x", (unsigned char)*byteptr++);
    }
    printf("\n");
}

void print_instruction(const char *instr_name, const char *op_src, char *op_dst, uint8_t *prefix_rex){
    printf("%s\t%%%s", instr_name, op_src);
    if(*prefix_rex > 0){
        *prefix_rex = 0;
    }
    printf("\n");
}
