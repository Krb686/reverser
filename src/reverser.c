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




/* Getopt structures */
static int flag_verbose;

struct option long_opts[] = {
    {"verbose", no_argument, &flag_verbose, 1},
    {"input-file", required_argument, 0, 'i'},
    {0, 0, 0, 0} 
};


static void print_elf64_half(char *prefix, ELF64_HALF *ptr){
    int i=0;
    printf("%s", prefix);
    for(i=0;i<2;i++){
        printf("i = %d", i);
        //printf("%02x ", *ptr);
        ptr+=2;
    }
    printf("\n");
}

static void print_elf64_word(char *prefix, ELF64_WORD *ptr){
    

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
void decode_file(FILE *p_file, struct ELF64_EHDR *p_elf_hdr){
    printf("%s\n", "Decoding the file...");    

    fread(&(p_elf_hdr->e_ident), 1, sizeof(p_elf_hdr->e_ident), p_file);
    
    size_t i;
    printf("e_ident = ");
    for (i = 0; i < sizeof(p_elf_hdr->e_ident); ++i){
        printf("%02x ", p_elf_hdr->e_ident[i]);
    }
    printf("\n");

    /* e_type */
    fread(&p_elf_hdr->e_type, 1, sizeof(p_elf_hdr->e_type), p_file);
    print_elf64_half("e_type = ", &(p_elf_hdr->e_type)); 

    /* e_machine */
    fread(&p_elf_hdr->e_machine, 1, sizeof(p_elf_hdr->e_machine), p_file);
    printf("e_machine = %04x\n", p_elf_hdr->e_machine);

    /* e_version */
    fread(&p_elf_hdr->e_version, 1, sizeof(p_elf_hdr->e_version), p_file);
    printf("e_version = x%" PRIx32 "\n", p_elf_hdr->e_version);

    /* e_entry */
    fread(&p_elf_hdr->e_entry, 1, sizeof(p_elf_hdr->e_entry), p_file);
    //char *p1 = uint64_to_char(p_elf_hdr->e_entry);
    //printf("e_entry = %s\n", p1);
    printf("e_entry = %0" PRIx64 "\n", p_elf_hdr->e_entry);

}

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
    decode_file(p_file, p_elf_hdr);

    exit(0);
}
