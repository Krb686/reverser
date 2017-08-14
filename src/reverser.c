#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <inttypes.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <unistd.h>

#include "reverser.h"
#include "darkelf.h"

#define EXIT_BAD_ARGS 1

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

#define DEBUG

#ifdef DEBUG
  #define PRINTD printf
#else
  #define PRINTD(format, args...) ((void)0)
#endif

// Getopt variables
static int flag_verbose;
struct option long_opts[] = {
    {"verbose", no_argument, &flag_verbose, 1},
    {"input-file", required_argument, 0, 'i'},
    {0,0,0,0}
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

char *ehdr;


// Begin
int main(int argc, char* argv[]){

    int c;
    int opt_index;
    char *filename;
    struct stat fstatbuf;

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
    if(filename == NULL){
        return 1;
    }
    int fd = open(filename, O_RDONLY);
    if (fstat(fd, &fstatbuf) != 0){
        perror("Error:");
    }
    // Memory map the file
    off_t filesize;
    filesize = lseek(fd, 0, SEEK_END);
    ehdr = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, fd, 0); 
    printf("ehdr located at %p\n", ehdr);
    decode_elf(ehdr);

    return 0;
}

void usage(){
    printf("Usage: reverse -i, --input <file>\n");
}
