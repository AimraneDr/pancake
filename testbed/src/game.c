#include "game.h"
#include <core/logger.h>
#include <core/pancake_memory.h>
#include <core\inputs.h>


b8 game_initialize(game* game_inst){
    PANCAKE_DEBUG("game_initialize() called")
    return true;
}

b8 game_update(game* game_inst,f32 delta_time){
    
    static u64 alloc_count = 0;
    u64 previouse_alloc_count = alloc_count;
    alloc_count = get_memory_allocations_count();
    if(input_key_is_up('M') && input_key_was_down('M')){
        PANCAKE_DEBUG("Allocations : %llu (%llu on this frame).", alloc_count, alloc_count - previouse_alloc_count);
    }
    return true;
}

b8 game_render(game* game_inst,f32 delta_time){
    return true;
}

void game_on_resize(game* game_inst,u32 width,u32 height){

}