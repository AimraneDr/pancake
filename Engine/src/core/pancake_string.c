#include "core/pancake_string.h"
#include "core/pancake_memory.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>   // isspace

#ifndef _MSC_VER
#include <strings.h>
#endif

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
    // b8 result = true;
    // for(int i=0;i < string_length(str0);i++){
    //     if(str0[i] != str1[i]){
    //         result  = false;
    //         break;
    //     }
    // }
    // return result;
    return strcmp(str0, str1) == 0;
}

i32 string_format(char* dest, const char* format, ...) {
    if (dest) {
        __builtin_va_list arg_ptr;
        va_start(arg_ptr, format);
        i32 written = string_format_v(dest, format, arg_ptr);
        va_end(arg_ptr);
        return written;
    }
    return -1;
}

i32 string_format_v(char* dest, const char* format, void* va_listp) {
    if (dest) {
        // Big, but can fit on the stack.
        char buffer[32000];
        i32 written = vsnprintf(buffer, 32000, format, va_listp);
        buffer[written] = 0;
        pancake_copy_memory(dest, buffer, written + 1);

        return written;
    }
    return -1;
}