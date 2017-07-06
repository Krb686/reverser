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

//TODO - objects to create
//  - elf header
//  - segment header table
//  - segment header table entry
//  - segment
//  - section header table
//  - section header table entry
//  - section

// Function prototypes
void usage();
void dump_ehdr(Elf64_Ehdr*);
void dump_phdr(Elf64_Phdr*, int );
void create_sections(Elf64_Shdr*, int);
void dump_sections(int);


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
const char *SECTION_TYPES[12] = { "SHT_NULL", "SHT_PROGBITS", "SHT_SYMTAB", "SHT_STRTAB", "SHT_RELA", "SHT_HASH", "SHT_DYNAMIC", "SHT_NOTE", "SHT_NOBITS", "SHT_REL", "SHT_SHLIB", "SHT_DYNSYM" };
const char *SECTION_FLAG_TYPES[8] = { "---", "write", "alloc", "alloc+write", "exec", "exec+write", "exec+alloc", "exec+alloc+write" };

Elf64_Ehdr *ehdr;

struct section {
    uint8_t  num;
    char     *name;
    uint8_t  type;
    char     *type_str;
    uint8_t  flags;
    char     *flags_str;
    uint64_t addr;
    uint64_t offset;
    uint64_t size;
    
    
    Elf64_Shdr *shdr;
    char     *bytes;
};

// Global arrays
struct section** __sections;

// Begin
int main(int argc, char* argv[]){

    int c;
    int opt_index;
    char *filename;
    char *incptr;
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
    printf("ehdr located at %p\n", ehdr);

    dump_ehdr(ehdr);


    // Locate and dump the program header table
    incptr = (char*)ehdr;
    phdr_array = (Elf64_Phdr*)(incptr + ehdr->e_phoff);
    printf("phdr_array located at %p\n", phdr_array);
    dump_phdr(phdr_array, ehdr->e_phnum);

    // Locate and dump the section header table
    shdr_array = (Elf64_Shdr*)(incptr + ehdr->e_shoff);
    printf("shdr_array = %p\n", shdr_array);
    __sections = malloc(sizeof(struct section*) * ehdr->e_shnum);
    create_sections(shdr_array, ehdr->e_shnum);
    dump_sections(ehdr->e_shnum);
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

        const char *segtype;
        if(phdr->p_type < 7){
            segtype = SEGMENT_TYPES[phdr->p_type];
        } else {
            segtype = "SPECIAL";
        }
        printf("\t\tp_type = 0x%" PRIx32 " = %s" "\n", phdr->p_type, segtype);

        const char *pflagtype;
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

void create_sections(Elf64_Shdr *shdr, int num_entries){
    // Get the string table entry
    // It is the section header table's address + (e_shstrndx * size of an entry)
    char *incptr = (char*)shdr;

    // Get the strtab address
    Elf64_Shdr *sh_strentry = (Elf64_Shdr*)(incptr + (ehdr->e_shstrndx * sizeof(Elf64_Shdr)));
    char *strtab = (char*)ehdr + sh_strentry->sh_offset;

    int i;
    for(i=0;i<num_entries;i++){
        struct section *s = malloc(sizeof(struct section));
        s->num = i;

        // Section name
        char *str_section = strtab + shdr->sh_name;
        s->name = malloc(strlen(str_section)+1);
        if(s->name == NULL){
            printf("error!\n");
        }
        strcpy(s->name, str_section);

        // Section type
        const char *sh_type_str;
        int sh_type = shdr->sh_type;
        if(sh_type < 12){
            sh_type_str = SECTION_TYPES[sh_type];
        } else if(sh_type >= SHT_LOPROC && sh_type <= SHT_HIPROC){
            sh_type_str = "PROCESSOR-SPECIFIC";
        } else if(shdr->sh_type >= SHT_LOUSER){
            sh_type_str = "APPLICATION-SPECIFIC";
        } else {
            sh_type_str = "UNKNOWN";
        }
        s->type = sh_type;
        s->type_str = malloc(strlen(sh_type_str)+1);
        strcpy(s->type_str, sh_type_str);

        // Section flags
        const char *sh_flags_str;
        uint8_t sh_flags = shdr->sh_flags & 0x7;
        sh_flags_str = SECTION_FLAG_TYPES[sh_flags];
        s->flags = sh_flags;
        s->flags_str = malloc(strlen(sh_flags_str)+1);
        strcpy(s->flags_str, sh_flags_str);

        s->addr = shdr->sh_addr;
        s->offset = shdr->sh_offset;
        s->size = shdr->sh_size;
        s->shdr = shdr;

        s->bytes = malloc(s->size);
        memcpy(s->bytes, (char*)ehdr + s->offset, s->size);

        __sections[i] = s;
        shdr++;
    }
}

void dump_sections(int num_entries){
    printf("======== SECTION HEADER TABLE ========\n");

    int i;
    printf("sizeof(__sections) = %d\n", sizeof(__sections));
    for(i=0;i<num_entries;i++){


        printf("--------\n");
        struct section *s = __sections[i];
        printf("s.num = %d\n", s->num);
        printf("s.name = %s\n", s->name);
        printf("s.type = %d\n", s->type);
        printf("s.type_str = %s\n", s->type_str);
        printf("s.flags = %d\n", s->flags);
        printf("s.flags_str = %s\n", s->flags_str);
        printf("s.addr = %" PRIx64 "\n", s->addr);
        printf("s.offset = %" PRIx64 "\n", s->offset);
        printf("s.size = %" PRIx64 "\n", s->size);

        int j;
        unsigned char *byte;
        for(j=0;j<s->size;j++){
            byte = s->bytes + j;
            //printf("byte addr = %x\n", byte);
            printf("%x", *byte);
        }
        printf("\n");

        /*
        printf("\t\tsh_name(index) = 0x%" PRIu32 " = %s" "\n", shdr->sh_name, str_section );
        printf("\t\tsh_type = 0x%" PRIx32 " = %s\n", sh_type, sh_type_str);
        printf("\t\tsh_flags = 0x%" PRIu64 " = %s\n", shdr->sh_flags, sh_flags_str);
        printf("\t\tsh_addr = 0x%" PRIx64 "\n", shdr->sh_addr);
        printf("\t\tsh_offset = 0x%" PRIx64 "\n", shdr->sh_offset);
        printf("\t\tsh_size = 0x%" PRIx64 "\n", shdr->sh_size);

        // Special sh_link handling
        printf("\t\tsh_link = 0x%" PRIx32 "\n", shdr->sh_link);
        if(sh_type == SHT_DYNAMIC || sh_type == SHT_SYMTAB || sh_type == SHT_DYNSYM){
            printf("\t\t\t--> section header table index to string table\n"); 
        } else if(sh_type == SHT_HASH || sh_type == SHT_REL || sh_type == SHT_RELA){
            printf("\t\t\t--> section header table index to symbol table\n");
        }
        

        // Special sh_info handling
        printf("\t\tsh_info = 0x%" PRIx32 "\n", shdr->sh_info);
        if(sh_type == SHT_REL || sh_type == SHT_RELA){
            printf("\t\t\t--> section header table index to relocatable section\n");
        }
        printf("\t\tsh_addralign = 0x%" PRIx64 "\n", shdr->sh_addralign);
        printf("\t\tsh_entsize = 0x%" PRIx64 "\n", shdr->sh_entsize);
        
        printf("\t\tCreating struct...\n");
        
        

        shdr++;
        */
    }

}

void usage(){
    printf("Usage: reverse -i, --input <file>\n");
}
