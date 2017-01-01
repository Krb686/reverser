#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>


#define EI_NIDENT     16
#define EI_MAG0       0
#define EI_MAG1       1
#define EI_MAG2       2
#define EI_MAG3       3
#define EI_CLASS      4
#define EI_DATA       5
#define EI_VERSION    6
#define EI_OSABI      7
#define EI_ABIVERSION 8

typedef uint64_t ELF64_ADDR;   // Unsigned program address
typedef uint64_t ELF64_OFF;    // Unsigned file offset
typedef uint16_t ELF64_HALF;   // Unsigned medium integer
typedef uint32_t ELF64_WORD;   // Unsigned integer
typedef int32_t  ELF64_SWORD;  // Signed integer
typedef uint64_t ELF64_XWORD;  // Unsigned long integer
typedef int64_t  ELF64_SXWORD; // Signed log integer

/* ELF Header */
struct ELF64_EHDR {

    unsigned char e_ident[16];
    ELF64_HALF    e_type;
    ELF64_HALF    e_machine;
    ELF64_WORD    e_version;
    ELF64_ADDR    e_entry;
    ELF64_OFF     e_phoff;
    ELF64_OFF     e_shoff;
    ELF64_WORD    e_flags;
    ELF64_HALF    e_ehsize;
    ELF64_HALF    e_phentsize;
    ELF64_HALF    e_phnum;
    ELF64_HALF    e_shentsize;
    ELF64_HALF    e_shnum;
    ELF64_HALF    e_shstrndx;

};

/* Program Header */
struct ELF64_PHDR_ENTRY {

    ELF64_WORD  p_type;
    ELF64_WORD  p_flags;
    ELF64_OFF   p_offset;
    ELF64_ADDR  p_vaddr;
    ELF64_ADDR  p_paddr;
    ELF64_XWORD  p_filesz;
    ELF64_XWORD  p_memsz;
    ELF64_XWORD  p_align;

};


const char *EI_CLASS_TYPES[3] = { "", "32-bit", "64-bit" };
const char *EI_DATA_TYPES[3]  = { "", "LSB", "MSB" };

#define EI_OSABI_TYPES_LEN 16
const char *EI_OSABI_TYPES[EI_OSABI_TYPES_LEN] = { "System V ABI", "HP-UX", "NetBSD", "Linux", "", "", "Solaris", "AIX", "IRIX", "FreeBSD", "", "", "OpenBSD", "OpenVMS", "NonStop Kernel", "AROS"};

#define E_TYPE_TYPES_LEN  5
const char *E_TYPE_TYPES[E_TYPE_TYPES_LEN]   = { "None", "Relocatable object file", "Executable file", "Shared object file", "Core file" };

#define E_MACHINE_TYPES_LEN 4
const char *E_MACHINE_TYPES[E_MACHINE_TYPES_LEN]  = {"None", "?", "SPARC", "x86"};



/* Getopt structures */
static int flag_verbose;

struct option long_opts[] = {
    {"verbose", no_argument, &flag_verbose, 1},
    {"input-file", required_argument, 0, 'i'},
    {0, 0, 0, 0} 
};


char* uint8_to_char(uint8_t val){
    static char buf[2];
    char *rptr = &buf[1]; 

    do {
        *rptr = "0123456789abcdef"[val % 16];
        --rptr;
        val /= 16;
    } while (val);
    return rptr;
}


char* uint16_to_char(uint16_t val){
    static char buf[7];
    buf[6]='\0';
    char *rptr = &buf[5];

    do {
        char a = "0123456789abcdef"[val % 16];
        //*rptr = "0123456789abcdef"[val % 16];
        printf("c = %c\n", a);
        --rptr;
        val /= 16;
    } while (val);
    *rptr-- = 'x';
    *rptr = '0';
    return rptr;
}

/*
char* uint32_to_char(uint32_t val){

}

char* uint64_to_char(uint64_t val){

}
*/

void exit_error(char *msg, int code){
    fprintf(stderr, msg);
    exit(code);
}

/* ================ Function: print_usage ================ */
/*                                                         */
/* ======================================================= */
void print_usage(){
    printf("%s\n", "Reverser 1.0");
    printf("%s\n", "Usage: reverser -i, --input-file FILE");
}

/* ================ Function: decode_file ================ */
/*                                                         */
/* ======================================================= */
void read_elf_header(FILE *p_file, struct ELF64_EHDR *ptr_ehdr){
    fread(&ptr_ehdr->e_ident,     1, sizeof(ptr_ehdr->e_ident),     p_file) == 16 ? 1  : exit_error("Error!", -1); /* e_ident   - 16 bytes */
    fread(&ptr_ehdr->e_type,      1, sizeof(ptr_ehdr->e_type),      p_file) == 2 ? 1   : exit_error("Error!", -1); /* e_type    - 2  bytes */
    fread(&ptr_ehdr->e_machine,   1, sizeof(ptr_ehdr->e_machine),   p_file) == 2 ? 1   : exit_error("Error!", -1); /* e_machine - 2 bytes */
    fread(&ptr_ehdr->e_version,   1, sizeof(ptr_ehdr->e_version),   p_file) == 4 ? 1   : exit_error("Error!", -1); /* e_version - 4 bytes */
    fread(&ptr_ehdr->e_entry,     1, sizeof(ptr_ehdr->e_entry),     p_file) == 8 ? 1   : exit_error("Error!", -1); /* e_entry   - 8 bytes */
    fread(&ptr_ehdr->e_phoff,     1, sizeof(ptr_ehdr->e_phoff),     p_file) == 8 ? 1   : exit_error("Error!", -1); /* e_phoff   - 8 bytes */
    fread(&ptr_ehdr->e_shoff,     1, sizeof(ptr_ehdr->e_shoff),     p_file) == 8 ? 1   : exit_error("Error!", -1); /* e_shoff   - 8 bytes */
    fread(&ptr_ehdr->e_flags,     1, sizeof(ptr_ehdr->e_flags),     p_file) == 4 ? 1   : exit_error("Error!", -1); /* e_flags   - 4 bytes */
    fread(&ptr_ehdr->e_ehsize,    1, sizeof(ptr_ehdr->e_ehsize),    p_file) == 2 ? 1   : exit_error("Error!", -1); /* e_ehsize  - 2 bytes */
    fread(&ptr_ehdr->e_phentsize, 1, sizeof(ptr_ehdr->e_phentsize), p_file) == 2 ? 1   : exit_error("Error!", -1); /* e_phentsize - 2 bytes */
    fread(&ptr_ehdr->e_phnum,     1, sizeof(ptr_ehdr->e_phnum),     p_file) == 2 ? 1   : exit_error("Error!", -1); /* e_phnum     - 2 bytes */
    fread(&ptr_ehdr->e_shentsize, 1, sizeof(ptr_ehdr->e_shentsize), p_file) == 2 ? 1   : exit_error("Error!", -1); /* e_shentsize - 2 bytes */
    fread(&ptr_ehdr->e_shnum,     1, sizeof(ptr_ehdr->e_shnum),     p_file) == 2 ? 1   : exit_error("Error!", -1); /* e_shnum     - 2 bytes */
    fread(&ptr_ehdr->e_shstrndx,  1, sizeof(ptr_ehdr->e_shstrndx),  p_file) == 2 ? 1   : exit_error("Error!", -1); /* e_shstrndx  - 2 bytes */
}

void read_phdr_entry(FILE *ptr_file, struct ELF64_PHDR_ENTRY *ptr_phdr_entry){
    fread(&ptr_phdr_entry->p_type,   1, sizeof(ptr_phdr_entry->p_type),   ptr_file) == 4 ? 1 : exit_error("Error!", -1);
    fread(&ptr_phdr_entry->p_flags,  1, sizeof(ptr_phdr_entry->p_flags),  ptr_file) == 4 ? 1 : exit_error("Error!", -1);
    fread(&ptr_phdr_entry->p_offset, 1, sizeof(ptr_phdr_entry->p_offset), ptr_file) == 8 ? 1 : exit_error("Error!", -1);
    fread(&ptr_phdr_entry->p_vaddr,  1, sizeof(ptr_phdr_entry->p_vaddr),  ptr_file) == 8 ? 1 : exit_error("Error!", -1);
    fread(&ptr_phdr_entry->p_paddr,  1, sizeof(ptr_phdr_entry->p_paddr),  ptr_file) == 8 ? 1 : exit_error("Error!", -1);
    fread(&ptr_phdr_entry->p_filesz, 1, sizeof(ptr_phdr_entry->p_filesz), ptr_file) == 8 ? 1 : exit_error("Error!", -1);
    fread(&ptr_phdr_entry->p_memsz,  1, sizeof(ptr_phdr_entry->p_memsz),  ptr_file) == 8 ? 1 : exit_error("Error!", -1);
    fread(&ptr_phdr_entry->p_align,  1, sizeof(ptr_phdr_entry->p_align),  ptr_file) == 8 ? 1 : exit_error("Error!", -1);
}


void read_phdr_table(FILE *ptr_file, struct ELF64_PHDR_ENTRY *ptr_phdr_array, int num_entries){
    int i;
    for (i=0;i<num_entries;i++){
        read_phdr_entry(ptr_file, ptr_phdr_array++);
    }
}

void dump_phdr_table(struct ELF64_PHDR_ENTRY *ptr_phdr_array, int num_entries){
    int i;
    for(i=0;i<num_entries;i++){
        printf("\tProgram header table entry #%d\n", i);
        printf("\t\tp_type = %"  PRIx32 "\n", ptr_phdr_array->p_type);
        printf("\t\tp_flags = %" PRIx32 "\n", ptr_phdr_array->p_flags);
        printf("\t\tp_offset = %" PRIx64 "\n", ptr_phdr_array->p_offset);
        printf("\t\tp_vaddr = %" PRIx64 "\n", ptr_phdr_array->p_vaddr);
        printf("\t\tp_paddr = %" PRIx64 "\n", ptr_phdr_array->p_paddr);
        printf("\t\tp_filesz = %" PRIx64 "\n", ptr_phdr_array->p_filesz);
        printf("\t\tp_memsz = %" PRIx64 "\n", ptr_phdr_array->p_memsz);
        printf("\t\tp_align = %" PRIx64 "\n", ptr_phdr_array->p_align);

        ptr_phdr_array++;
    }
}



void dump_elf_header(struct ELF64_EHDR *ptr_ehdr){

    printf("Dumping the elf header...\n");
    printf("\te_ident = ");
    int i;
    for (i = 0; i < sizeof(ptr_ehdr->e_ident); ++i){
        printf("%02x ", ptr_ehdr->e_ident[i]);
    }
    printf("\n");

    printf("\te_ident fields:\n");
    printf("\t\te_ident[4] = EI_CLASS      = %02x --> %s\n", ptr_ehdr->e_ident[EI_CLASS], EI_CLASS_TYPES[ptr_ehdr->e_ident[EI_CLASS]]);
    printf("\t\te_ident[5] = EI_DATA       = %02x --> %s\n", ptr_ehdr->e_ident[EI_DATA],  EI_DATA_TYPES[ptr_ehdr->e_ident[EI_DATA]]);
    printf("\t\te_ident[6] = EI_VERSION    = %02x\n", ptr_ehdr->e_ident[EI_VERSION]);
    printf("\t\te_ident[7] = EI_OSABI      = %02x --> %s\n", ptr_ehdr->e_ident[EI_OSABI], EI_OSABI_TYPES[ptr_ehdr->e_ident[EI_OSABI]]);
    printf("\t\te_ident[8] = EI_ABIVERSION = %02x\n", ptr_ehdr->e_ident[EI_ABIVERSION]);
    printf("\t\te_ident[9-15] = PADDING BYTES\n");

    int e_type_val = ptr_ehdr->e_type;
    printf("\te_type = %04x", e_type_val);
    if (e_type_val > E_TYPE_TYPES_LEN) {
        printf(" --> UNKNOWN!\n");
    } else {
        printf(" --> %s\n", E_TYPE_TYPES[ptr_ehdr->e_type]);
    }

    int e_machine_val = ptr_ehdr->e_machine;
    printf("\te_machine = %04x", e_machine_val);
    if (e_machine_val > E_MACHINE_TYPES_LEN){
        printf(" --> UNKNOWN!\n");
    } else {
        printf(" --> %s\n", E_MACHINE_TYPES[e_machine_val]);
    }

    printf("\te_version = %08x\n", ptr_ehdr->e_version);
    printf("\te_entry = %" PRIx64 "\n", ptr_ehdr->e_entry);
    printf("\te_phoff = %" PRIx64 "\n", ptr_ehdr->e_phoff);
    printf("\te_shoff = %" PRIx64 "\n", ptr_ehdr->e_shoff);
    printf("\te_flags = %08x\n", ptr_ehdr->e_flags);
    printf("\te_ehsize = %04x\n", ptr_ehdr->e_ehsize);
    printf("\te_phentsize = %04x\n", ptr_ehdr->e_phentsize);
    printf("\te_phnum = %04x\n", ptr_ehdr->e_phnum);
    printf("\te_shentsize = %04x\n", ptr_ehdr->e_shentsize);
    printf("\te_shnum = %04x\n", ptr_ehdr->e_shnum);
    printf("\te_shstrndx = %04x\n", ptr_ehdr->e_shstrndx);
}


/* ================ Function: main ============================ */
/*                                                              */
/* ============================================================ */
int main( int argc, char **argv){

    int c;
    int opt_index = 0;
    char *p_file_name;

        
    if(argc == 1){
       print_usage(); 
       exit(0);
    } else {
        while((c = getopt_long(argc, argv, "i:v", long_opts, &opt_index)) != -1) {
            switch(c){
                case 'i':
                    p_file_name = strdup(optarg);
                    break;
                case '?':
                    fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
                    return(-2);
                default:
                    fprintf(stderr, "%s: invalid option -- %c\n", argv[0], c);
                    fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
                    return(-2);
            }
        }

        if (flag_verbose){
            printf("%s\n", "Verbose is ON!");
        }
    }

    FILE *p_file = fopen(p_file_name, "r");
    if(p_file == NULL){
        fprintf(stderr, "Cannot open %s\n", p_file_name);
    }


    struct ELF64_EHDR elf_hdr, *p_ehdr;
    p_ehdr = &elf_hdr;


    /* Read elf header */
    read_elf_header(p_file, p_ehdr);
    dump_elf_header(p_ehdr);


    if(p_ehdr->e_type == 2 && p_ehdr->e_phnum > 0) {

        struct ELF64_PHDR_ENTRY ptr_phdr_array[p_ehdr->e_phnum];
        printf("Read the program header table\n");
        read_phdr_table(p_file, ptr_phdr_array, p_ehdr->e_phnum);

        dump_phdr_table(ptr_phdr_array, p_ehdr->e_phnum);

    } 

    exit(0);
}
