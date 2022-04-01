#include "pancake_memory.h"
#include "core/logger.h"
#include "core/pancake_string.h"
#include "platform/platform.h"
#include <stdio.h>


struct memory_stats{
    u64 total_allocated;                            //track down the total allocated memory
    u64 tagged_allocations[MEMORY_TAG_MAX_TAGS];    //track down the total allocated memory for each memory tag
};

static const char* memory_tags_string[MEMORY_TAG_MAX_TAGS] = {
    "UNKNOWN            ",
    "LINEAR_ALLOCATION  ",
    "ARRAY              ",
    "LIST               ",
    "DICT               ",
    "RING_QUEUE         ",
    "BST                ",
    "STRING             ",
    "AAPLICATION        ",
    "JOB                ",
    "TEXTURE            ",
    "MATERIAL_INSTANCE  ",
    "RENDERER           ",
    "GAME               ",
    "TRANSFORM          ",
    "ENTITY             ",
    "ENTITY_NODE        ",
    "SCENE              "
};

typedef struct memory_system_state{
    struct memory_stats stats;
    u64 allocations_count;
}memory_system_state;

static memory_system_state* state_ptr;


void initialize_memory_system(u64* required_memory, void* state){
    *required_memory = sizeof(memory_system_state);
    if(state == 0){
        return;
    }
    state_ptr = state;
    state_ptr->allocations_count = 0;
    platform_zero_memory(&state_ptr->stats,sizeof(state_ptr->stats));
}
void shutdown_memory_system(void* state){
    state_ptr = 0;
}

void* pancake_allocate(u64 size, memory_tag tag){
    if(tag == MEMORY_TAG_UNKNOWN){
        PANCAKE_WARN("allocate called using MEMORY_TAG_UNKNOWN , Re-class this allocation");
    }

    if(state_ptr){
        state_ptr->stats.total_allocated += size;
        state_ptr->stats.tagged_allocations[tag] += size;
        state_ptr->allocations_count++;
    }

    //TODO: memory alignement
    void* block = platform_allocate(size,false);
    platform_zero_memory(block, size);
    return block;
}
void pancake_free(void* block, u64 size, memory_tag tag){
    if(tag == MEMORY_TAG_UNKNOWN){
        PANCAKE_WARN("allocate called using MEMORY_TAG_UNKNOWN , Re-class this allocation");
    }
    if(state_ptr){
        state_ptr->stats.total_allocated -= size;
        state_ptr->stats.tagged_allocations[tag] -= size;
    }

    //TODO: memory alignement
    platform_free(block, false);
}
void* pancake_zero_memory(void* block, u64 size){
    return platform_zero_memory(block,size);
}
void* pancake_copy_memory(void* dest, const void* source, u64 size){
    return platform_copy_memory(dest,source,size);
}
void* pancake_set_memory(void* dest, i32 value, u64 size){
    return platform_set_memory(dest,value,size);
}
char* get_memory_usage_str(){
    const u64 Gb = 1024 * 1024 * 1024;
    const u64 Mb = 1024 * 1024;
    const u64 Kb = 1024;

    char buffer[8000] = "system memor usage (tagged) :\n";
    u64 offset = string_length(buffer);
    for(i32 i=0; i < MEMORY_TAG_MAX_TAGS; ++i){
        char unit[3] = "Xb";
        float amount = 1.0f;
        if(state_ptr->stats.tagged_allocations[i] >= Gb){
            unit[0] = 'G';
            amount = state_ptr->stats.tagged_allocations[i] / (float)Gb;
        }else if(state_ptr->stats.tagged_allocations[i] >= Mb){
            unit[0] = 'M';
            amount = state_ptr->stats.tagged_allocations[i] / (float)Mb;
        }else if(state_ptr->stats.tagged_allocations[i] >= Kb){
            unit[0] = 'K';
            amount = state_ptr->stats.tagged_allocations[i] / (float)Kb;
        }else{
            unit[0] = 'B';
            unit[1] = 0;
            amount = state_ptr->stats.tagged_allocations[i];
        }
        //snprintf() returns the number of character written
        //so we add the return value to our offset variable
        offset += snprintf(buffer + offset, 8000, "\t%s : %.2f %s\n", memory_tags_string[i], amount, unit);
    }

    char* out_string = string_duplicate(buffer);
    return out_string;
}
u64 get_memory_allocations_count(){
    if(state_ptr)   return state_ptr->allocations_count;
    return 0;
}