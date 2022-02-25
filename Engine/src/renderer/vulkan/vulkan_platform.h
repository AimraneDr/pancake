#pragma once
#include "defines.h"

/*
appends the required extensions names to tge names_list, which should be created and passed in.
*/
void platform_get_required_extensions(const char*** names_list);
//NOTE: the implementation of this interface is in "engine/src/platform/platform_win32.c" file