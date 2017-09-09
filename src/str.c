#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "str.h"

void reverse_str(char *str){
    if(str == NULL){
        return;
    }

    char *start = str;
    char *end = str + strlen(str) - 2;
    char temp;
    char temp2;

    while(end > start){
        temp = *start;
        temp2 = *(start+1);

        *start = *end;
        *(start+1) = *(end+1);

        *end = temp;
        *(end+1) = temp2;

        start = start + 2;
        end = end - 2;;
    }
}

char *trim_leading(char *str, char trimchar){
    char *start = str;
    char *copy = str;

    // safety check
    if(str == NULL || str[0] == '\0'){
        return str;
    }

    // move the start ptr beyond the set of trimchars
    while(start[0] == trimchar){
        start++;
    }

    // Copy the string left
    while(*start){
        *copy++ = *start++;
    }
    *copy = '\0';


    return str;
}


uint8_t is_hex(char *str){
    if(strspn(str, "0123456789abcdefABCDEF") == strlen(str)){
        printf("returning 1...\n");
        return 1;
    } else {
        printf("returning 0...\n");
        return 0;
    }
}

// options for sign extending
// 1) convert to numeric, extend, convert back to string
// 2) convert to binary string, string extend, convert back to hex string
// 3) 
char* sign_extend(char *str, uint8_t bitlen){
    uint8_t delta;

    // String must contain only hex characters
    if(is_hex(str)){
        // String length must be less than desired extended length
        if(strlen(str)*4 < bitlen){
            delta = bitlen - strlen(str)*4;
            char *binstr = malloc(strlen(str) + 1);
            binstr = hex2bin(str);
            printf("binstr = %s\n", binstr);
            printf("need to extend by %d bits\n", delta);

            // Find the uppermost bit
            // This converts the character to a number
            long topbit = strtol((char[]){str[0], 0}, NULL, 16) >> 3;
            printf("topbit = %ld\n", topbit);
          
            // Convert the number back to a character
            char c[2]; 
            snprintf(c, 2, "%d", (int)topbit); 
            printf("conv c = %s\n", c);
            int i;
            char *extend_str = malloc(delta + strlen(str)*4 + 1);
            char *newstr = malloc(delta + 1);
            // Create the sign extended string
            for(i=0;i<delta;i++){
                strcat(extend_str, c);
            }
            strcat(extend_str, binstr);
            printf("extend_str = %s\n", extend_str);

            char *newhexstr = bin2hex(extend_str, (uint8_t)SIGNED);
            printf("newhexstr = %s\n", newhexstr);
            return newhexstr;
        }
    }
}

char *bin2hex(char *binstr, uint8_t sign){
    uint8_t binlen = strlen(binstr);
    printf("binlen = %d\n", binlen);
    uint8_t padlen = 4 - (binlen % 4);
    if(padlen == 4){
        padlen = 0;
    }
    printf("padlen = %d\n", padlen);
    uint8_t hexlen = binlen + padlen;
    printf("hexlen = %d\n", hexlen);

    uint8_t i;
    char *binstr_full = malloc(hexlen);
    if(padlen > 0){
        for(i=0;i<padlen;i++){
            if(sign == UNSIGNED){
                strcat(binstr_full, "0");
            } else if(sign == SIGNED){
                strcat(binstr_full, binstr[0]);
            }
        }
    }
    strcat(binstr_full, binstr);
    printf("binstr_full = %s\n", binstr_full);


    char *c = binstr_full;
    uint64_t binval = 0;
    while(*c){
        int b = *c=='1'?1:0;
        binval = (binval<<1)|b;
        c++;
    }
    char *hexstr = malloc((hexlen / 4) + 1);
    snprintf(hexstr, (hexlen / 4)+1, "%lx", binval);
    return hexstr;
}

char *hex2bin(char *hexstr){

    const char *bindigits[24] = { "0000", "0001", "0010", "0011", "0100", "0101", "0110", "0111", \
                                  "1000", "1001", "1010", "1011", "1100", "1101", "1110", "1111", \
                                  "1000", "1001", "1010", "1011", "1100", "1101", "1110", "1111"
                                  };
    char *hexdigits = "0123456789abcdefABCDEF";
    char *binstr = malloc(strlen(hexstr)*4 + 1);
    char *c = hexstr;

    // Loop over the hex string
    while(*c){
        long v = strtol((char[]){*c++, 0}, NULL, 16);
        strcat(binstr, bindigits[v]);
    }
    return binstr;
}
