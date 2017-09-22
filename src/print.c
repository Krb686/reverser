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


void print_instruction(struct instr *i){

    const char *func_addr = i->operand_dst;
    char *func_name = "hi";
    switch(i->operand_fmt){
    //register-register
    case OP_FMT_RR:
        printf("\t\t\t%d) %s\t%%%s,%%%s\n", ++instr_number, i->name, i->operand_src, i->operand_dst);
        break;
    //immediate-register
    case OP_FMT_IR:
        printf("\t\t\t%d) %s\t$0x%s,%%%s\n", ++instr_number, i->name, i->operand_src, i->operand_dst);
        break;
    //register src
    case OP_FMT_Rs:
        printf("\t\t\t%d) %s\t%%%s\n", ++instr_number, i->name, i->operand_src);
        break;
    //register dst
    case OP_FMT_Rd:
        printf("\t\t\t%d) %s\t%%%s\n", ++instr_number, i->name, i->operand_dst);
        break;
    //address
    case OP_FMT_A:
        printf("\t\t\t%d) %s %s <%s>\n", ++instr_number, i->name, func_addr, func_name);
        break;
    //none
    case OP_FMT_N:
        printf("\t\t\t%d) %s\n", ++instr_number, i->name);
        break;
    default:
        printf("\t\t\t%d) %s\n", ++instr_number, i->name);
        break;
    }
}

void print_instructions(struct state_core *sc){
    struct instr *i = sc->instr_list;
    while(i){
        print_instruction(i);
        i = i->n;
    }
}
