#include <stdio.h>

#include "dis.h"
#include "print.h"

static int instr_number = 0;

void print_bytes(char *byteptr, int n){
    int i;
    for(i=0;i<n;i++){
        printf("%02x", (unsigned char)*byteptr++);
    }
    printf("\n");
}

void print_instruction(const int format, const char *instr_name, const char *op_src, const char *op_dst, uint8_t *prefix_rex){

    switch(format){
    case OP_FMT_RR:
        printf("\t\t\t%d) %s\t%%%s,%%%s", ++instr_number, instr_name, op_src, op_dst);
        break;
    case OP_FMT_IR:
        printf("\t\t\t%d) %s\t$%s,%%%s", ++instr_number, instr_name, op_src, op_dst);
        break;
    case OP_FMT_R:
        break;
    case OP_FMT_A:
        break;
    case OP_FMT_N:
        break;
    }
    
    if(*prefix_rex > 0){
        *prefix_rex = 0;
    }
    printf("\n");
}
