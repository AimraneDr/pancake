#pragma once

#include "defines.h"

typedef enum memory_tag{
    MEMORY_TAG_UNKNOWN,     //for temporary use, should be assigned to one of the tags below or create a new one .
    MEMORY_TAG_LINEAR_ALLOCATION,
    MEMORY_TAG_ARRAY,
    MEMORY_TAG_LIST,
    MEMORY_TAG_DICT,
    MEMORY_TAG_RING_QUEUE,
    MEMORY_TAG_BST,
    MEMORY_TAG_STRING,
    MEMORY_TAG_AAPLICATION,
    MEMORY_TAG_JOB,
    MEMORY_TAG_TEXTURE,
    MEMORY_TAG_MATERIAL_INSTANCE,
    MEMORY_TAG_RENDERER,
    MEMORY_TAG_GAME,
    MEMORY_TAG_TRANSFORM,
    MEMORY_TAG_ENTITY,
    MEMORY_TAG_ENTITY_NODE,
    MEMORY_TAG_SCENE,

    MEMORY_TAG_MAX_TAGS
}memory_tag;

PANCAKE_API void initialize_memory_system(u64* required_memory, void* state);
PANCAKE_API void shutdown_memory_system(void* state);

PANCAKE_API void* pancake_allocate(u64 size, memory_tag tag);
PANCAKE_API void pancake_free(void* block, u64 size, memory_tag tag);
PANCAKE_API void* pancake_zero_memory(void* block, u64 size);
PANCAKE_API void* pancake_copy_memory(void* dest, const void* source, u64 size);
PANCAKE_API void* pancake_set_memory(void* dest, i32 value, u64 size);
PANCAKE_API char* get_memory_usage_str();
PANCAKE_API u64 get_memory_allocations_count();