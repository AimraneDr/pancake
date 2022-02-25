#include "core/pancake_string.h"
#include "core/pancake_memory.h"

#include <string.h>

u64 string_length(const char* str) {
    return strlen(str);
}

char* string_duplicate(const char* str) {
    u64 length = string_length(str);
    char* copy = pancake_allocate(length + 1, MEMORY_TAG_STRING);
    pancake_copy_memory(copy, str, length + 1);
    return copy;
}


b8 strings_equal(const char* str0, const char* str1){
    // b8 result = TRUE;
    // for(int i=0;i < string_length(str0);i++){
    //     if(str0[i] != str1[i]){
    //         result  = FALSE;
    //         break;
    //     }
    // }
    // return result;
    return strcmp(str0, str1) == 0;
}
