#ifndef DARKELF_H
#define DARKELF_H
#include <elf.h>
char* addr2funclabel(char*);
void create_sections(Elf64_Ehdr*, Elf64_Shdr*, int);
void decode_section(Elf64_Ehdr*, int);
void decode_elf(char*);
void dump_ehdr(Elf64_Ehdr*);
void dump_phdr(Elf64_Phdr*, int );
void dump_sections(Elf64_Ehdr*);
void resolve_function_labels();
#endif
