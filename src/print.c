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


void print_instruction(struct state_core *sc){
//void print_instruction(const int format, const char *instr_name, const char *op_src, const char *op_dst){


    const char *func_addr = sc->operand_dst;
    char *func_name = "hi";
    printf("FMT = %d\n", sc->operand_fmt);
    switch(sc->operand_fmt){
    case OP_FMT_RR:
        printf("\t\t\t%d) %s\t%%%s,%%%s", ++instr_number, sc->instr_name, sc->operand_src, sc->operand_dst);
        break;
    case OP_FMT_IR:
        printf("\t\t\t%d) %s\t$0x%s,%%%s", ++instr_number, sc->instr_name, sc->operand_src, sc->operand_dst);
        break;
    case OP_FMT_Rs:
        printf("\t\t\t%d) %s\t%%%s", ++instr_number, sc->instr_name, sc->operand_src);
        break;
    case OP_FMT_Rd:
        printf("\t\t\t%d) %s\t%%%s", ++instr_number, sc->instr_name, sc->operand_dst);
        break;
    case OP_FMT_A:
        
        printf("\t\t\t%d) %s %s <%s>", ++instr_number, sc->instr_name, func_addr, func_name);
        break;
    case OP_FMT_N:
        printf("\t\t\t%d) %s\n", ++instr_number, sc->instr_name);
        break;
    default:
        printf("\t\t\t%d) %s\n", ++instr_number, sc->instr_name);
        break;
    }
    
    printf("\n");
}
