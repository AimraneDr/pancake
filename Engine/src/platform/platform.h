#pragma once
#include "defines.h"

typedef struct platform_state{
    void*  internal_state;
}platform_state;

b8 platform_startup(
    platform_state* plat_state,
    char* application_name,
    i32 x,
    i32 y,
    i32 width,
    i32 height
);

void shutdown_platform(platform_state* plat_state);

b8 platform_pump_messages(platform_state* state);

void* platform_allocate(u64 size,b8 aligned);
void platform_free(void* block,b8 aligned);
void* platform_zero_memory(void* block, u64 size);
void* platform_copy_memory(void* dest,const void* source,u64 size);
void* platform_set_memory(void* dest,i32 value,u64 size);

void platform_console_write(const char* msg, u8 colour);
void platform_console_write_error(const char* msg, u8 colour);

f64 platform_get_absolute_time();

//sleep on the thread for the provided ammount of ms, this block the main thread
//should only be used to give time back to the OS for unused update power
//there for it isn't exported.
void platform_sleep(u64 ms);
