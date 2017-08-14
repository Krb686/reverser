#ifndef DARKELF_H
#define DARKELF_H
#include <elf.h>
void dump_ehdr(Elf64_Ehdr*);
void dump_phdr(Elf64_Phdr*, int );
void create_sections(Elf64_Shdr*, int);
void dump_sections(int);
void decode_section(int);
void decode_elf(char*);
#endif
