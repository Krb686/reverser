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

#define OPCODE       1
#define NOP          2
#define DISPLACEMENT 3
#define MODRM        4
#define SIB          5
#define OPERAND      6

#define BYTE2BINPAT "%c%c%c%c%c%c%c%c"
#define BYTE2BIN(byte)       \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0')

//TODO - objects to create
//  - elf header
//  - segment header table
//  - segment header table entry
//  - segment
//  - section header table
//  - section header table entry
//  - section
//
//  - .gnu.hash decode
//  - .rela.dyn meaning

// Notes
// .interp - path to interpreter
// .note
//   - namesz - # of bytes in name containing string representing entry owner/originator
//   - descsz - # of bytes in desc containg the note descriptor
//   - type - interpretation of descriptor. Implementation/owner specific
//   - name
//   - desc
//
//   .ABI-tag
//     - name = 'GNU\0'
//     - type = 1
//     - descz >= 16
//     - desc - 1st 32bit word = 0 (linux executeable), 2+3+4 = earliest compatible kernel version

// Function prototypes
void usage();
void dump_ehdr(Elf64_Ehdr*);
void dump_phdr(Elf64_Phdr*, int );
void create_sections(Elf64_Shdr*, int);
void dump_sections(int);
void decode_section(int);
void print_bytes(char*, int);
void decode_instructions(unsigned char*, int);
void add_instr(const char*);
void add_prefix(char*);
void change_state(int);
void print_instruction(char*, uint8_t*);


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
    uint32_t link;
    uint32_t info;
    uint64_t addralign;
    uint64_t entsize;
    
    
    Elf64_Shdr *shdr;
    unsigned char     *bytes;
};

struct symbol {
    char *name;
    uint8_t version;
    uint8_t type;
};

struct instr {
    char *name;
    char *bytes;
    struct section *sptr;
    uint64_t offset;
};

// Global arrays
struct section** __sections;
struct symbol**  __symbols;

int __bytenum = 0;
int __opfound = 0;

int instr_count = 0;

const char *INSTR_NAMES[3][256] = 
{ 
    {
        "", "add", "", "", "", "", "", "", "", "", "", "", "", "", "", "-",    \
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "pop",  \
        "", "", "", "", "", "", "", "", "sub", "sub", "sub", "sub", "sub", "sub", "", "",      \
        "", "xor", "", "", "", "", "", "", "", "", "", "", "", "", "", "",      \
        "REX", "REX", "REX", "REX", "REX", "REX", "REX", "REX", "REX", "REX", "REX", "REX", "REX", "REX", "REX", "REX",        \
        "push", "", "", "push", "push", "push", "push", "push", "", "", "", "", "", "pop", "pop", "",         \
        "", "", "", "", "", "", "-", "", "", "", "", "", "", "", "", "",         \
        "", "", "", "", "je", "jne", "", "ja", "", "", "", "", "", "", "", "",         \
        "-", "", "", "-", "", "test", "", "", "", "mov", "", "", "", "lea", "", "",         \
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "", "mov", "mov", "mov", "mov", "mov", "mov", "mov", "mov",         \
        "", "-", "", "ret", "", "", "-", "-", "", "", "", "", "", "", "", "",         \
        "", "-", "", "", "", "", "", "", "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "", "call", "jmp", "", "", "", "", "", "",         \
        "", "", "", "-", "hlt", "", "", "", "", "", "", "", "", "", "", "-"
    },
    {
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",    \
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "nop",         \
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",      \
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",      \
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",        \
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""
    }, 
    {
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",    \
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",      \
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",      \
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",        \
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",         \
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""
    }
};

const int STATE_NEXT_MAP[3][256] = 
{ 
    {
        9, MODRM, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, MODRM, 9, 9, 9, OPERAND, 9, 9, \
        9, MODRM, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, OPCODE, 9, 9, 9, 9, 9, 9, OPCODE, OPCODE, 9, 9, OPCODE, 9, 9, 9, \
        OPCODE, 9, 9, OPCODE, OPCODE, OPCODE, OPCODE, OPCODE, 9, 9, 9, 9, 9, OPCODE, OPCODE, 9, \
        9, 9, 9, 9, 9, 9, OPCODE, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, OPERAND, OPERAND, 9, OPERAND, 9, 9, 9, 9, 9, 9, 9, 9, \
        MODRM, 9, 9, MODRM, 9, MODRM, 9, 9, 9, MODRM, 9, 9, 9, MODRM, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, MODRM, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, OPERAND, 9, OPERAND, 9, 9, 9, 9, OPERAND, \
        9, MODRM, 9, OPCODE, 9, 9, MODRM, MODRM, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, MODRM, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, OPERAND, DISPLACEMENT, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, OPCODE, OPCODE, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, MODRM 
    },
    {
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, MODRM, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9 
    },
    {
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, \
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9 
    }
};



const char *INSTR_NAMES_GROUP1[8] = { "add", "or", "adc", "sbb", "and", "sub", "xor", "cmp" };
const char *INSTR_NAMES_GROUP2[8] = { "rol", "ror", "rcl", "rcr", "shl", "shr", "", "sar" };
const char *INSTR_NAMES_GROUP5[8] = { "inc", "dec", "call", "call", "jmp", "jmp", "push", "" };
const char *INSTR_NAMES_GROUP11[8] = { "mov", "",   "",    "",    "",    "",    "",    "xbegin" };
const char *REX_STRS[16] = { "----", "---B", "--X-", "--XB", "-R--", "-R-B", "-RX-", "-RXB", "W---", "W--B", "W-X-", "W-XB", "WR--", "WR-B", "WRX-", "WRXB", };


int OPERAND_BYTES[8] = { 0, 0, 0, 0, 0, 0, 0, 0};
int DISPLACEMENT_BYTES = 0;
const char *STATE_NEXT_STRINGS[7] = { "", "OPCODE", "", "", "MODRM", "SIB", "OPERAND" };



int __state_next  = OPCODE;




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

    // Allocate memory for section pointer array
    __sections = malloc(sizeof(struct section*) * ehdr->e_shnum);

    // Create the section structs
    create_sections(shdr_array, ehdr->e_shnum);

    // Dump contents of the section structs
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
        } else if(sh_type == SHT_GNU_verdef){
            sh_type_str = "SHT_GNU_verdef";
        } else if(sh_type == SHT_GNU_verneed){
            sh_type_str = "SHT_GNU_verneed";
        } else if(sh_type == SHT_GNU_versym){
            sh_type_str = "SHT_GNU_versym";
        
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
        s->link = shdr->sh_link;
        s->info = shdr->sh_info;
        s->addralign = shdr->sh_addralign;
        s->entsize = shdr->sh_entsize;

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
    printf("sizeof(__sections) = %" PRIu64 "\n", sizeof(__sections));
    for(i=0;i<num_entries;i++){


        printf("-------- Section Attributes --------\n");
        struct section *s = __sections[i];
        printf("\ts.num = %d\n", s->num);
        printf("\ts.name = %s\n", s->name);
        printf("\ts.type = %x\n", s->type);
        printf("\ts.type_str = %s\n", s->type_str);
        printf("\ts.flags = %d\n", s->flags);
        printf("\ts.flags_str = %s\n", s->flags_str);
        printf("\ts.addr = %" PRIx64 "\n", s->addr);
        printf("\ts.offset = %" PRIx64 "\n", s->offset);
        printf("\ts.size = %" PRIx64 "\n", s->size);
        printf("\ts.link = %" PRIx32 "\n", s->link);
        printf("\ts.info = %" PRIx32 "\n", s->info);
        printf("\ts.addralign = %" PRIx64 "\n", s->addralign);
        printf("\ts.entsize = %" PRIx64 "\n", s->entsize);

        int j;
        unsigned char *byte;
        printf("\tsection bytes:\n");
        for(j=0;j<s->size;j++){
            byte = s->bytes + j;
            //printf("byte addr = %x\n", byte);
            if(j%2 == 0 && j != 0){
                printf(" ");
            }
            if(j == 0){
                printf("\t\t");
            } else if(j%64 == 0){
                printf("\n\t\t");
            }
            printf("%02x", *byte);
        }
        printf("\n");

        /*

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
        */


        decode_section(s->num);
    }

}

void decode_section(int s_num){
    printf("\tdecoding section #%d\n", s_num);

    struct section *s = __sections[s_num];

    // Note sections
    if(strcmp(s->type_str, "SHT_NOTE") == 0){
        uint32_t namesz, descsz, type;
        char *name, *desc;

        printf("\t\t---- SHT_NOTE Generic ----\n");
        // Copy namesz, descsz, and type bytes that are common to all 'note' sections
        memcpy(&namesz, s->bytes, 4);
        printf("\t\t\tnamesz = %" PRIu32 "\n", namesz);
        memcpy(&descsz, (s->bytes + 4), 4);
        printf("\t\t\tdescsz = %" PRIu32 "\n", descsz);
        memcpy(&type, (s->bytes + 8), 4);
        printf("\t\t\ttype = %" PRIu32 "\n", type);

        // Allocate and copy name
        name = malloc(namesz);
        memcpy(name, (s->bytes + 12), namesz);
        printf("\t\t\tname = %s\n", name);

        // Allocate and copy desc
        desc = malloc(descsz);
        memcpy(desc, (s->bytes + 12 + namesz), descsz);
        printf("\t\t\tdesc = ");
        print_bytes(desc, descsz);

        printf("\t\t---- Section Specific ----\n");
        // .note.ABI-tag
        if(strcmp(s->name, ".note.ABI-tag") == 0){
            uint32_t data1, data2, data3, data4;
            memcpy(&data1, s->bytes + 12 + namesz, 4); 
            memcpy(&data2, s->bytes + 12 + namesz + 4, 4);
            memcpy(&data3, s->bytes + 12 + namesz + 8, 4);
            memcpy(&data4, s->bytes + 12 + namesz + 12, 4);

            printf("\t\t\tdata1 = %" PRIu32 "\n", data1);
            printf("\t\t\tdata2 = %" PRIu32 "\n", data2);
            printf("\t\t\tdata3 = %" PRIu32 "\n", data3);
            printf("\t\t\tdata4 = %" PRIu32 "\n", data4);

            if(data1 == 0){
                printf("\t\t\tdata block 1: 0 --> executable\n");
            } else {
                printf("\t\t\tdata block 1: UNKNOWN!\n");
            }
            printf("\t\t\tdata blocks 2-4: earliest compatible kernel = %" PRIu32 ".%" PRIu32 ".%" PRIu32 "\n", data2, data3, data4);
            //
        // .note.gnu.build-id
        } else if(strcmp(s->name, ".note.gnu.build-id") == 0){
            printf("\t\t\tSHA1 Build ID: ");
            print_bytes(desc, descsz);
        }
        
    // Normal and Dynamic symbol table
    } else if(strcmp(s->type_str, "SHT_DYNSYM") == 0 || strcmp(s->type_str, "SHT_SYMTAB") == 0){
        Elf64_Sym *sym = (Elf64_Sym*)s->bytes;
        struct section *s_strtab = __sections[s->link];
        int i;
        for(i=0;i<s->size/24;i++){
            printf("\t\t---- symbol #%d ----\n", i);
            printf("\t\t\tst_name (index) = %08" PRIx32 " --> %s\n", sym->st_name, s_strtab->bytes+sym->st_name);
            printf("\t\t\tst_info = %02" PRIu8 "\n", sym->st_info);
            printf("\t\t\tst_other = %02" PRIu8 "\n", sym->st_other);
            printf("\t\t\tst_shndx = %04" PRIx16 "\n", sym->st_shndx);
            printf("\t\t\tst_value = %016" PRIx64 "\n", sym->st_value);
            printf("\t\t\tst_size = %016" PRIx64 "\n", sym->st_size);
            sym++;
        }
    // String tables
    } else if(strcmp(s->type_str, "SHT_STRTAB") == 0){
        printf("\t\tstrings:\n");

        // Loop over strings in the SHT_STRTAB contents
        unsigned char *base = s->bytes+1; // skip over the initial null string
        unsigned char *str = base;
        while(str < base + s->size-1){
            printf("\t\t\t%s\n", str);
            str = str + strlen((const char *)str) + 1;
       }
    } else if(strcmp(s->type_str, "SHT_GNU_versym") == 0){
        int i;
        int num_entries = s->size / 2;
        for(i=0;i<num_entries;i++){
            printf("\t\tsymbol version: %" PRIu16 "\n", *(s->bytes + 2*i));
        }
    } else if(strcmp(s->type_str, "SHT_GNU_verneed") == 0){
        int num = s->size / sizeof(Elf64_Verneed);
        Elf64_Verneed *verneed = (Elf64_Verneed *)s->bytes;
        int i;
        for(i=0;i<num;i++){
            printf("\t\t---- verneed #%d ----\n", i);
            printf("\t\t\tvn_version = %" PRIu16 "\n", verneed->vn_version);
            printf("\t\t\tvn_cnt = %" PRIu16 "\n", verneed->vn_cnt);
            printf("\t\t\tvn_file = %" PRIu32 "\n", verneed->vn_file);
            printf("\t\t\tvn_aux = %" PRIu32 "\n", verneed->vn_aux);
            printf("\t\t\tvn_next = %" PRIu32 "\n", verneed->vn_next);
            verneed++;
        }
    } else if(strcmp(s->type_str, "SHT_RELA") == 0){
        if(strcmp(s->name, ".rela.dyn") == 0){
            Elf64_Rela *rela = (Elf64_Rela*)s->bytes;
            printf("\t\t---- relocation ----\n");
            printf("\t\t\tr_offset = %" PRIu64 "\n", rela->r_offset);
            printf("\t\t\tr_info = 0x%" PRIx64 "\n", rela->r_info);
            printf("\t\t\tr_addend = 0x%" PRIx64 "\n", rela->r_addend);
        } else if(strcmp(s->name, ".rela.plt") == 0){
            int num = s->size / sizeof(Elf64_Rela);
            int i;
            Elf64_Rela *rela = (Elf64_Rela*)s->bytes;
            for(i=0;i<num;i++){
                printf("\t\t---- relocation ----\n");
                printf("\t\t\tr_offset = %" PRIu64 "\n", rela->r_offset);
                printf("\t\t\tr_info = 0x%" PRIx64 "\n", rela->r_info);
                printf("\t\t\tr_addend = 0x%" PRIx64 "\n", rela->r_addend);
                rela++;
            }
        }
    } else {
        if(strcmp(s->name, ".interp") == 0){
            printf("\t\tinterpreter = %s\n", s->bytes);
        } else if(strcmp(s->name, ".text") == 0){
            printf("\t\tdecoding instructions\n");
            decode_instructions(s->bytes, s->size);
        }
    }
}

void print_bytes(char *byteptr, int n){
    int i;
    for(i=0;i<n;i++){
        printf("%02x", (unsigned char)*byteptr++);
    }
    printf("\n");
}

void decode_instructions(unsigned char *byte, int numbytes){
    __bytenum = 0;
    int argbytes = 0;
    int modrm = 0;

    uint8_t opcode_sz = 1;

    unsigned char *i_start = byte;
    int i_len = 0;
    const char *instr_name;

    int groupnum = 0;

    uint8_t modefield = 0;
    uint8_t regfield = 0;
    uint8_t rmfield = 0;

    uint8_t prefix_rex = 0;
    uint8_t prefix_oper = 0;

    // TODO - make this into more of a state machine
    int __state       = OPCODE;
    
    int flag_nop = 0;
    int flag_skip_opcode = 0;
    int num_operands = 0;
    int op_bytes_index = 0;
    int displacement = 0;
    int nop_len = 0;

    // Loop over all bytes
    while(__bytenum < numbytes){
        printf("\t\t\t\tbyte = %x\n", *byte);
        switch(__state){
            case OPCODE:
                switch(*byte){
                case 0x01:
                    num_operands = 1;
                    OPERAND_BYTES[0] = 1;
                    break;
                case 0x0f:
                    if(opcode_sz == 1){
                        opcode_sz++; 
                        flag_skip_opcode = 1;
                    }
                    break;
                case 0x1f:
                   if(opcode_sz == 2){
                       num_operands = 1;
                       flag_nop = 1;
                   }
                   break;
                case 0x29:
                    num_operands = 0;
                    break;
                case 0x2d:
                    num_operands = 1;
                    OPERAND_BYTES[0] = 4;
                    break;
                case 0x31: //xor
                    break;
                case 0x41:
                    prefix_rex = *byte & 0xF;
                    break;
                case 0x44:
                    break;
                case 0x47:
                    prefix_rex = *byte & 0xF;
                    break;
                case 0x48:
                    prefix_rex = *byte & 0xF;
                    break;
                case 0x49:
                    prefix_rex = *byte & 0xF;
                    break;
                case 0x4c:
                    prefix_rex = *byte & 0xF;
                    break;
                case 0x50:
                    //mark_instr("push");
                    break;
                case 0x55:
                    //mark_instr("push");
                    break;
                case 0x66:
                    prefix_oper = 1;
                    break;
                case 0x74:
                    num_operands = 1;
                    OPERAND_BYTES[0] = 1;
                    break;
                case 0x75:
                    num_operands = 1;
                    OPERAND_BYTES[0] = 1;
                    break;
                case 0x77:
                    num_operands = 1;
                    OPERAND_BYTES[0] = 1;
                    break;
                case 0x80:
                    groupnum = 1;
                    OPERAND_BYTES[0] = 1;
                    break;
                case 0x83:
                    printf("\t\t\t\tgroup 1\n");
                    groupnum = 1;
                    // Ev, Ib (1 modrm byte, 1 imm byte)
                    OPERAND_BYTES[0] = 1;
                    break;
                case 0x85:
                    break;
                case 0x89:
                    break;
                case 0xba:
                    num_operands = 1;
                    OPERAND_BYTES[0] = 4;
                    break;
                case 0xb8:
                    OPERAND_BYTES[0] = 4;
                    num_operands = 1;
                    break;
                case 0xbf:
                    num_operands = 1;
                    OPERAND_BYTES[0] = 4;
                    break;
                case 0x5d:
                    //mark_instr("pop");
                    break;
                case 0x5e:
                    //mark_instr("pop");
                    break;
                case 0xc1:
                    groupnum = 2;
                    num_operands = 1;
                    OPERAND_BYTES[0] = 1;
                    break;
                case 0xc3:
                    //mark_instr("retq");
                    break;
                case 0xc6:
                    //group 11, Eb, Ib
                    groupnum = 11;
                    num_operands = 1;
                    OPERAND_BYTES[0] = 1;
                    break;
                case 0xc7:
                    groupnum = 11;
                    printf("\t\t\t\tgroup 11\n");
                    // Ev, Iz (1 modrm byte, 1 immediate word (16 bit operand size) / double word (32 or 64 bit operand size)
                    OPERAND_BYTES[0] = 4;
                    break;
                case 0xd1:
                    groupnum = 2;
                    num_operands = 0;
                    break;
                case 0xe8:
                    num_operands = 1;
                    OPERAND_BYTES[0] = 4;
                    break;
                case 0xe9:
                    DISPLACEMENT_BYTES = 4;
                    printf("num_ops = %d\n", num_operands);
                    break;
                case 0xf4:
                    break;
                case 0xff:
                    groupnum = 5;
                    break;
                }

                // Skip assigning an instruction for the current opcode on special bytes (opcode size increase)
                if(!flag_skip_opcode){
                    printf("opcode_sz = %d\n", opcode_sz);
                    instr_name = INSTR_NAMES[opcode_sz - 1][*byte];

                    if(instr_name != NULL && strcmp(instr_name, "") == 0){
                        //printf("instr not implemented!\n");
                        printf("unaccounted byte - exiting\n");
                        return;
                    } else {
                        if(strcmp(instr_name, "REX") == 0 || strcmp(instr_name, "-") == 0){
                            // instruction can't be determined yet
                        } else {
                            print_instruction(instr_name, &prefix_rex);
                        }
                        
                        change_state(STATE_NEXT_MAP[opcode_sz - 1][*byte]);
                    }
                }

                break;
            case DISPLACEMENT:
                printf("DISPLACEMENT\n");
                if(DISPLACEMENT_BYTES > 0){
                    DISPLACEMENT_BYTES--;

                    if(DISPLACEMENT_BYTES == 0){
                        if(num_operands > 0){
                            change_state(OPERAND);
                            opcode_sz = 1;
                        } else {
                            change_state(OPCODE);
                        }
                        displacement = 0;
                    }
                }
                break;
            case MODRM:
                printf("MODRM\n");
                modefield = (*byte >> 6) & 0x3;
                regfield = (*byte >> 3) & 0x7;
                rmfield = (*byte) & 0x7;

                if(modefield == 0 && rmfield == 5){
                    // modrm = 00xxx101
                    //displacement 32 bit
                    displacement = 1;
                    DISPLACEMENT_BYTES = 4;
                }


                if(groupnum > 0){
                    

                    switch(groupnum){

                    case 1:
                        instr_name = INSTR_NAMES_GROUP1[regfield];
                        if(instr_name != NULL && strcmp(instr_name, "") == 0){
                            //printf("instr not implemented!\n");
                        } else {
                            print_instruction(instr_name, &prefix_rex);
                        }

                        num_operands = 1;
                        groupnum = 0;
                        break;
                    case 2:
                        instr_name = INSTR_NAMES_GROUP2[regfield];
                         if(instr_name != NULL && strcmp(instr_name, "") == 0){
                            printf("instr not implemented!\n");
                        } else {
                            print_instruction(instr_name, &prefix_rex);
                        }
                        groupnum = 0;
                        break;
                    case 5:
                        instr_name = INSTR_NAMES_GROUP5[regfield];
                        if(instr_name != NULL && strcmp(instr_name, "") == 0){
                            printf("instr not implemented!\n");
                        } else {
                            print_instruction(instr_name, &prefix_rex);
                        }
                        groupnum = 0;
                        break;
                    case 11:
                        instr_name = INSTR_NAMES_GROUP11[regfield];
                        // TODO - remove this duplicate code
                        if(instr_name != NULL && strcmp(instr_name, "") == 0){
                            printf("instr not implemented!\n");
                        } else {
                            print_instruction(instr_name, &prefix_rex);
                        }

                        num_operands = 1;
                        groupnum = 0;
                        break;
                    }
                }

                if(flag_nop){
                        switch(*byte){
                            case 0x00:
                                num_operands = 0;
                                break;
                            case 0x40:
                                OPERAND_BYTES[0] = 1;
                                break;
                            case 0x44:
                                OPERAND_BYTES[0] = 2;
                                break;
                            case 0x80:
                                OPERAND_BYTES[0] = 4;
                                break;
                        }
                        flag_nop = 0;
                    }
                

                if(displacement == 1){
                    change_state(DISPLACEMENT);
                } else if(num_operands > 0){
                    change_state(OPERAND);
                } else {
                    change_state(OPCODE);
                    opcode_sz = 1;
                }
                break;
            case NOP:
                if(nop_len > 0){
                    nop_len--;
                    if(nop_len == 0){
                        change_state(OPCODE);
                    }
                }
                break;
            case OPERAND:
                printf("CASE --> OPERAND\n");
                printf("num_operands = %d\n", num_operands);
                if(num_operands > 0){

                    if(OPERAND_BYTES[op_bytes_index] > 0){ 
                        OPERAND_BYTES[op_bytes_index]--;
                    }

                    // Reduce operand count when operand bytes have been consumed
                    if(OPERAND_BYTES[op_bytes_index] == 0){
                        num_operands--;
                        op_bytes_index++;
                    }

                    if(num_operands == 0){
                        change_state(OPCODE);
                        op_bytes_index = 0;
                        opcode_sz = 1;
                    } else {
                        if(groupnum == 1){
                            
                        }
                        
                    }

                        
                } else {
                    change_state(OPCODE);
                    op_bytes_index = 0;
                    opcode_sz = 1;
                    // Reset flags
                    groupnum = 0;
                }

                
                break;
        }

        
        byte++;
        __bytenum++;
        flag_skip_opcode = 0;
        // Assign the new state
        __state = __state_next;
    }
}

void add_instr(const char *name){
    //struct instr *i = malloc(sizeof(struct instr));
    //i->name = malloc(strlen(name)+1);        
    //i->bytes = malloc();
    

    printf("\tname = %s, byte #%d\n", name, __bytenum);
    __opfound = 0;
}

void add_prefix(char *prefix){
    printf("\t\t\tprefix = %s\n", prefix);
}

void usage(){
    printf("Usage: reverse -i, --input <file>\n");
}

void change_state(int index){
    __state_next = index;
    printf("\t\t\t\tnext state --> %s\n", STATE_NEXT_STRINGS[index]);
}

void print_instruction(char *instr_name, uint8_t *prefix_rex){
    instr_count++;                    
    printf("\t\t\t%d) - %s", instr_count, instr_name);
    if(*prefix_rex > 0){
        printf("(REX: %s)", REX_STRS[*prefix_rex]);
        *prefix_rex = 0;
    }
    printf("\n");
}
