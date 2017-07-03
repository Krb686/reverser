#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <inttypes.h>
#include <elf.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <unistd.h>
#define EXIT_BAD_ARGS 1

// Function prototypes
void usage();
void dump_ehdr(Elf64_Ehdr *ehdr);
void dump_phdr(Elf64_Phdr *phdr, int num_entries);


// Getopt variables
static int flag_verbose;
struct option long_opts[] = {
    {"verbose", no_argument, &flag_verbose, 1},
    {"input-file", required_argument, 0, 'i'},
    {0,0,0,0}
};

// ELF formats
const char *EI_CLASS_TYPES[3] = { "", "32-bit", "64-bit" };
const char *EI_DATA_TYPES[3]  = { "", "LSB", "MSB" };
const char *EI_OSABI_TYPES[16] = { "System V ABI", "HP-UX", "NetBSD", "Linux", "", "", "Solaris", "AIX", "IRIX", "FreeBSD", "", "", "OpenBSD", "OpenVMS", "NonStop Kernel", "AROS" };
const char *SEGMENT_TYPES[8] = { "PT_NULL", "PT_LOAD", "PT_DYNAMIC", "PT_INTERP", "PT_NOTE", "PT_SHLIB", "PT_PHDR", "" };
const char *SEGMENT_FLAG_TYPES[8] = { "---", "--X", "-W-", "-WX", "R--", "R-X", "RW-", "RWX" };

Elf64_Ehdr *ehdr;

// Begin
int main(int argc, char* argv[]){

    int c;
    int opt_index;
    char *filename;
    struct stat fstatbuf;
    Elf64_Phdr *phdr_array;
    Elf64_Shdr *shdr_array;

    // Handle arguments
    if(argc == 1){
        usage();
        exit(EXIT_BAD_ARGS);
    }
    while ((c = getopt_long(argc, argv, "i:v", long_opts, &opt_index)) != -1){
        switch(c){
            case 'i':
                filename = strdup(optarg);
                printf("filename = %s\n", filename);
        }
    }

    // Open the file
    int fd = open(filename, O_RDONLY);
    if (fstat(fd, &fstatbuf) != 0){
        perror("Error:");
    }
    // Memory map the file
    off_t filesize;
    filesize = lseek(fd, 0, SEEK_END);
    ehdr = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, fd, 0); 

    dump_ehdr(ehdr);


    // Locate and dump the program header table
    phdr_array = (char*)ehdr + ehdr->e_phoff;
    dump_phdr(phdr_array, ehdr->e_phnum);

    // Locate and dump the section header table
    shdr_array = (char*)ehdr + ehdr->e_shoff;
    printf("shdr_array = 0x%" PRIx64 "\n", shdr_array);
    dump_shdr(shdr_array, ehdr->e_shnum);
    return 0;
}

void dump_ehdr(Elf64_Ehdr *ehdr){
    printf("======== ELF HEADER ========\n");
    printf("\te_ident:\n");
    int i;
    for(i=0;i<sizeof(ehdr->e_ident);++i){
        printf("\t\t%02x\n", ehdr->e_ident[i]);
    }
    printf("\n");
    printf("\te_type =      0x%" PRIx16 "\n", ehdr->e_type);
    printf("\te_machine =   0x%" PRIx16 "\n", ehdr->e_machine);
    printf("\te_version =   0x%" PRIx32 "\n", ehdr->e_version);
    printf("\te_entry =     0x%" PRIx64 "\n", ehdr->e_entry);
    printf("\te_phoff =     0x%" PRIx64 "\n", ehdr->e_phoff);
    printf("\te_shoff =     0x%" PRIx64 "\n", ehdr->e_shoff);
    printf("\te_flags =     0x%" PRIx32 "\n", ehdr->e_flags);
    printf("\te_ehsize =    0x%" PRIx16 "\n", ehdr->e_ehsize);
    printf("\te_phentsize = 0x%" PRIx16 "\n", ehdr->e_phentsize);
    printf("\te_phnum =     0x%" PRIx16 "\n", ehdr->e_phnum);
    printf("\te_shentsize = 0x%" PRIx16 "\n", ehdr->e_shentsize);
    printf("\te_shnum =     0x%" PRIx16 "\n", ehdr->e_shnum);
    printf("\te_shstrndx =  0x%" PRIx16 "\n", ehdr->e_shstrndx);
}

void dump_phdr(Elf64_Phdr *phdr, int num_entries){
    printf("======== PROGRAM HEADER TABLE ========\n");
    int i;
    for(i=0;i<num_entries;i++){
        printf("\tEntry #%d\n", i+1);

        char *segtype;
        if(phdr->p_type < 7){
            segtype = SEGMENT_TYPES[phdr->p_type];
        } else {
            segtype = "SPECIAL";
        }
        printf("\t\tp_type = 0x%" PRIx32 " = %s" "\n", phdr->p_type, segtype);

        char *pflagtype;
        if(phdr->p_flags < 8){
            pflagtype = SEGMENT_FLAG_TYPES[phdr->p_flags];
        } else {
            pflagtype = "???";
        }
        printf("\t\tp_flags = 0x%" PRIx32 " = %s" "\n", phdr->p_flags, pflagtype);
        printf("\t\tp_offset = 0x%" PRIx64 "\n", phdr->p_offset);
        printf("\t\tp_vaddr = 0x%" PRIx64 "\n", phdr->p_vaddr);
        printf("\t\tp_paddr = 0x%" PRIx64 "\n", phdr->p_vaddr);
        printf("\t\tp_filesz = 0x%" PRIx64 "\n", phdr->p_filesz);
        printf("\t\tp_memsz = 0x%" PRIx64 "\n", phdr->p_memsz);
        printf("\t\tp_align = 0x%" PRIx64 "\n", phdr->p_align);
        phdr++;
    }
}

void dump_shdr(Elf64_Shdr *shdr, int num_entries){
    printf("======== SECTION HEADER TABLE ========\n");

    // Get the string table entry
    // It is the section header table's address + (e_shstrndx * size of an entry)
    Elf64_Shdr *sh_strentry = ((char*)shdr + (ehdr->e_shstrndx * sizeof(Elf64_Shdr)));
    printf("\t\tsh_strentry = 0x%" PRIx64 "\n", sh_strentry);

    // Get the strtab address
    char *strtab = (char*)ehdr + sh_strentry->sh_offset;
    printf("\t\tstrtab = 0x%" PRIx64 "\n", strtab);

    int i;
    for(i=0;i<num_entries;i++){
        printf("\tSection #%d\n", i);
        printf("\t\tshdr located at - 0x%" PRIx64 "\n", shdr);
        char *str_section = strtab + shdr->sh_name;
        printf("\t\tsh_name(index) = 0x%" PRIu32 " = %s" "\n", shdr->sh_name, str_section );
        printf("\t\tsh_type = 0x%" PRIx32 "\n", shdr->sh_type);
        printf("\t\tsh_flags = 0x%" PRIx32 "\n", shdr->sh_flags);
        printf("\t\tsh_addr = 0x%" PRIx64 "\n", shdr->sh_addr);
        printf("\t\tsh_offset = 0x%" PRIx64 "\n", shdr->sh_offset);
        shdr++;
    }


}

void usage(){
    printf("Usage: reverse -i, --input <file>\n");
}
