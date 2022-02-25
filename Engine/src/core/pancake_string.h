#pragma once

#include "defines.h"

// Returns the length of the given string.
PANCAKE_API u64 string_length(const char* str);
PANCAKE_API char* string_duplicate(const char* str); 

//case sensative string comparison, true if the same otherwise false 
PANCAKE_API b8 strings_equal(const char* str0, const char* str1);