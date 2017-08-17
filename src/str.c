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
