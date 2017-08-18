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
