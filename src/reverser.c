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
#define EI_PAD        9

typedef uint64_t ELF64_ADDR;
typedef uint64_t ELF64_OFF;
typedef uint16_t ELF64_HALF;
typedef uint32_t ELF64_WORD;
typedef int32_t  ELF64_SWORD;
typedef int64_t  ELF64_XWORD;
typedef int64_t  ELF64_SXWORD;

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
struct ELF64_PHDR {

    ELF64_WORD  p_type;
    ELF64_OFF   p_offset;
    ELF64_ADDR  p_vaddr;
    ELF64_ADDR  p_paddr;
    ELF64_WORD  p_filesz;
    ELF64_WORD  p_memsz;
    ELF64_WORD  p_flags;
    ELF64_WORD  p_align;

};



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
void read_elf_header(FILE *p_file, struct ELF64_EHDR *p_elf_hdr){
    printf("%s\n", "Decoding the file...");    

    /* e_ident */
    int bytes_read, i;
    bytes_read = fread(&(p_elf_hdr->e_ident), 1, sizeof(p_elf_hdr->e_ident), p_file);
    if(bytes_read != 16){
        fprintf(stderr, "Error reading e_ident!");
        exit(-1);
    }
    printf("e_ident = ");
    for (i = 0; i < sizeof(p_elf_hdr->e_ident); ++i){
        printf("%02x ", p_elf_hdr->e_ident[i]);
    }
    printf("\n");

    /* e_type - 2 bytes*/
    bytes_read = fread(&p_elf_hdr->e_type, 1, sizeof(p_elf_hdr->e_type), p_file);
    if(bytes_read != 2){
        fprintf(stderr, "Error reading e_type! Read %d bytes instead of 4!\n", bytes_read);
        exit(-1);
    }
    //char *ptr1 = uint16_to_char(p_elf_hdr->e_type);
    printf("e_type = %04x\n", p_elf_hdr->e_type);

    /* e_machine - 2 bytes*/
    fread(&p_elf_hdr->e_machine, 1, sizeof(p_elf_hdr->e_machine), p_file);
    printf("e_machine = %04x\n", p_elf_hdr->e_machine);

    /* e_version - 4 bytes*/
    fread(&p_elf_hdr->e_version, 1, sizeof(p_elf_hdr->e_version), p_file);
    printf("e_version = %08x\n", p_elf_hdr->e_version);

    /* e_entry - 8 bytes*/
    fread(&p_elf_hdr->e_entry, 1, sizeof(p_elf_hdr->e_entry), p_file);
    //char *p1 = uint64_to_char(p_elf_hdr->e_entry);
    printf("e_entry = %" PRIx64 "\n", p_elf_hdr->e_entry);

    /* e_phoff - 8 bytes */
    fread(&p_elf_hdr->e_phoff, 1, sizeof(p_elf_hdr->e_phoff), p_file);
    printf("e_phoff = %" PRIx64 "\n", p_elf_hdr->e_phoff);

    /* e_shoff - 8 bytes */
    fread(&p_elf_hdr->e_shoff, 1, sizeof(p_elf_hdr->e_shoff), p_file);
    printf("e_shoff = %" PRIx64 "\n", p_elf_hdr->e_shoff);

    /* e_flags - 4 bytes */
    fread(&p_elf_hdr->e_flags, 1, sizeof(p_elf_hdr->e_flags), p_file);
    printf("e_flags = %08x\n", p_elf_hdr->e_flags);

    /* e_ehsize */
    fread(&p_elf_hdr->e_ehsize, 1, sizeof(p_elf_hdr->e_ehsize), p_file);
    printf("e_ehsize = %04x\n", p_elf_hdr->e_ehsize);

    /* e_phentsize */
    fread(&p_elf_hdr->e_phentsize, 1, sizeof(p_elf_hdr->e_phentsize), p_file);
    printf("e_phentsize = %04x\n", p_elf_hdr->e_phentsize);

    /* e_phnum */
    fread(&p_elf_hdr->e_phnum, 1, sizeof(p_elf_hdr->e_phnum), p_file);
    printf("e_phnum = %04x\n", p_elf_hdr->e_phnum);

    /* e_shentsize */
    fread(&p_elf_hdr->e_shentsize, 1, sizeof(p_elf_hdr->e_shentsize), p_file);
    printf("e_shentsize = %04x\n", p_elf_hdr->e_shentsize);

    /* e_shnum */
    fread(&p_elf_hdr->e_shnum, 1, sizeof(p_elf_hdr->e_shnum), p_file);
    printf("e_shnum = %04x\n", p_elf_hdr->e_shnum);

    /* e_shstrndx */
    fread(&p_elf_hdr->e_shstrndx, 1, sizeof(p_elf_hdr->e_shstrndx), p_file);
    printf("e_shstrndx = %04x\n", p_elf_hdr->e_shstrndx);

}


void read_program_header(FILE *ptr_file, struct ELF64_PHDR *ptr_phdr){

    int bytes_read;
    /* p_type - 4 bytes */
    bytes_read = fread(&ptr_phdr->p_type, 1, sizeof(ptr_phdr->p_type), ptr_file);
    if (bytes_read != 4){
        fprintf(stderr, "Error reading phdr->p_type");
        exit(-1);
    }
    //printf("p_type = %08x\n", ptr_phdr->p_type);
    printf("p_type = %" PRIx32 "\n", ptr_phdr->p_type);

    /* p_offset - 8 bytes */
    bytes_read = fread(&ptr_phdr->p_offset, 1, sizeof(ptr_phdr->p_offset), ptr_file);
    if (bytes_read != 8){
        fprintf(stderr, "Error reading phdr->p_offset");
        exit(-1);
    }
    //printf("p_offset = %016x\n", ptr_phdr->p_offset);
    printf("p_offset = %" PRIx64 "\n", ptr_phdr->p_offset);

    /* p_vaddr - 8 bytes */
    bytes_read = fread(&ptr_phdr->p_vaddr, 1, sizeof(ptr_phdr->p_vaddr), ptr_file);
    if (bytes_read != 8){
        fprintf(stderr, "Error reading phdr->p_vaddr");
        exit(-1);
    }
    //printf("p_vaddr = %016hhx\n", ptr_phdr->p_vaddr);
    printf("p_vaddr = %" PRIx64 "\n", ptr_phdr->p_vaddr);

    /* p_paddr - 8 bytes */
    bytes_read = fread(&ptr_phdr->p_paddr, 1, sizeof(ptr_phdr->p_paddr), ptr_file);
    if (bytes_read != 8){
        fprintf(stderr, "Error reading phdr->p_paddr");
        exit(-1);
    }
    //printf("p_paddr = %016hhx\n", ptr_phdr->p_paddr);
    printf("p_paddr = %" PRIx64 "\n", ptr_phdr->p_paddr);

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


    struct ELF64_EHDR elf_hdr, *p_elf_hdr;
    p_elf_hdr = &elf_hdr;


    /*Decode bytes from the file */
    read_elf_header(p_file, p_elf_hdr);

    if(p_elf_hdr->e_type == 2) {

        struct ELF64_PHDR phdr, *p_phdr;
        p_phdr = &phdr;

        printf("Read the program header...\n");
        read_program_header(p_file, p_phdr);
    } 

    exit(0);
}
