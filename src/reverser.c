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
#define OPCODE 0
#define MODRM  1
#define SIB    2

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
void add_instr(char*);


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

const char *INSTR_NAMES[256] = { "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "nop_m",    \
                                 "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",         \
                                 "", "", "", "", "", "", "", "", "", "", "", "", "", "sub", "", "",      \
                                 "", "xor", "", "", "", "", "", "", "", "", "", "", "", "", "", "",      \
                                 "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",        \
                                 "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",         \
                                 "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",         \
                                 "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",         \
                                 "", "", "", "", "", "", "", "", "", "mov", "", "", "", "", "", "",         \
                                 "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",         \
                                 "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",         \
                                 "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",         \
                                 "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",         \
                                 "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",         \
                                 "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",         \
                                 "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""         };



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

        int offset = 0;
        char *base = s->bytes+1; // skip over the initial null string
        while(offset < s->size-1){
            printf("\t\t\t%s\n", (base + offset));
            offset = offset + strlen(base+offset) + 1;
       }
    } else if(strcmp(s->type_str, "SHT_GNU_versym") == 0){
        int i;
        int num_entries = s->size / 2;
        for(i=0;i<num_entries;i++){
            printf("\t\tsymbol version: %" PRIu16 "\n", (uint16_t*)*(s->bytes + 2*i));
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
            printf("decoding instructions\n");
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
    int opsize_override = 0;
    int nopm = 0;
    int modrm = 0;


    char *i_start = byte;
    int i_len = 0;
    char *i_name;

    int group1 = 0;
    int __curbyte_t = 0;
    int __nexbyte_t = 0;

    uint8_t modefield = 0;
    uint8_t regfield = 0;
    uint8_t rmfield = 0;
    while(__bytenum < numbytes){
        if(__curbyte_t == OPCODE){
        //if(__opfound == 0){
            switch(*byte){
                case 0x0f:
                    
                    //mark_instr(iname, "nop_m");
                    nopm = 1;
                    break;
                case 0x1f:
                   if(nopm == 1){

                   } 
                   break;
                case 0x2d:
                    //mark_instr("sub");
                    argbytes = 4;
                    break;
                case 0x31: //xor
                    __nexbyte_t = MODRM;
                    break;
                case 0x44:
                    if(nopm == 1){
                        argbytes = 2;
                        __opfound = 0;
                    }
                    break;
                case 0x47:
                    add_prefix("REX.-RXB");
                    break;
                case 0x48:
                    add_prefix("REX.W---");
                    break;
                case 0x49:
                    add_prefix("REX.W--B");
                    break;
                case 0x50:
                    //mark_instr("push");
                    break;
                case 0x55:
                    //mark_instr("push");
                    break;
                case 0x66:
                    add_prefix("operand size override");
                    opsize_override = 1;
                    break;
                case 0x74:
                    //TODO - this could also be 'jz'
                    //mark_instr("je");
                    argbytes = 1;
                    break;
                case 0x77:
                    //mark_instr("ja");
                    break;
                case 0x83:
                    group1 = 1;
                    __opfound = 1;
                    modrm = 1;
                    break;
                case 0x85:
                    //mark_instr("test");
                    break;
                case 0x89:
                    //mark_instr("mov");
                    break;
                case 0xb8:
                    //mark_instr("mov");
                    argbytes = 4;
                    break;
                case 0x5d:
                    //mark_instr("pop");
                    break;
                case 0x5e:
                    //mark_instr("pop");
                    break;
                case 0xc3:
                    //mark_instr("retq");
                    break;
                case 0xc7:
                    //mark_instr("mov");
                    argbytes = 5;
                    break;
                case 0xe8:
                    //mark_instr("call");
                    argbytes = 4;
                    break;
                case 0xf4:
                    //mark_instr("hlt");
                    break;
            }

            i_name = INSTR_NAMES[*byte];

            if(i_name != NULL && strcmp(i_name, "") == 0){
                //printf("instr not implemented!\n");
            } else {
                printf("i_name = %s\n", i_name);
                __opfound = 1;
            }

            //}
            
        } else {

            // Group 1 instructions are defined by the 1st byte + bits 3-5 of byte 2
            if(group1 == 1){
                if(modrm == 1){
                    modefield = (*byte >> 6) & 0x3;
                    regfield = (*byte >> 3) & 0x7;
                    rmfield = (*byte) & 0x7;

                    printf("\t\tmodefield = %" PRIu8 "\n", modefield);
                    printf("\t\tregfield = %" PRIu8 "\n", regfield);
                    printf("\t\trmfield = %" PRIu8 "\n", rmfield);


                    switch(regfield){
                        case 0x00:
                            //mark_instr("add");
                            break;
                        case 0x01:
                           // mark_instr("or");
                            break;
                        case 0x02:
                            //mark_instr("adc");
                            break;
                        case 0x03:
                            //mark_instr("sbb");
                            break;
                        case 0x04:
                            //mark_instr("and");
                            break;
                        case 0x05:
                            //mark_instr("sub");
                        case 0x06:
                            //mark_instr("xor");
                            break;
                        case 0x07:
                            //mark_instr("cmp");
                            break;
                    }
                    modrm = 0;

                    
                }
                group1 = 0;
                
            } else {
                
                printf("\t\targ byte = 0x%" PRIx8 "\n", (unsigned char)*byte);
                printf("\t\targbytes = %d\n", argbytes);
                if(argbytes > 0){
                    argbytes--;

                    if(argbytes == 0){
                        __opfound = 0;
                        opsize_override = 0;
                        nopm = 0;
                        add_instr(i_name);
                    }
                } else {
                    __opfound = 0;
                }
            }
        }

        byte++;
        __bytenum++;
    }
}

void add_instr(char *name){
    //struct instr *i = malloc(sizeof(struct instr));
    //i->name = malloc(strlen(name)+1);        
    //i->bytes = malloc();
    

    printf("\tname = %s, byte #%d\n", name, __bytenum);
    __opfound = 0;
}

void add_prefix(char *prefix){
    printf("\tprefix = %s\n", prefix);
}

void usage(){
    printf("Usage: reverse -i, --input <file>\n");
}
