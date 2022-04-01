#include "renderer_frontend.h"
#include "renderer_backend.h"
#include "core/logger.h"
#include "core/pancake_memory.h"

typedef struct renderer_system_state {
    renderer_backend backend;
} renderer_system_state;

static renderer_system_state* state_ptr;


b8 initialize_renderer_system(u64* memory_requirement, void* state, const char* application_name){
    *memory_requirement = sizeof(renderer_system_state);
    if (state == 0) {
        return true;
    }
    state_ptr = state;

    //TODO: make this configurable
    renderer_backend_create(RENDERER_BACKEND_TYPE_VULKAN, &state_ptr->backend);
    state_ptr->backend.frame_number = 0;

    if(!state_ptr->backend.initialize(&state_ptr->backend,application_name)){
        PANCAKE_FATAL("Rendeerer backend failed to initialize , shutting down...");
        return false;
    }


    return true;
}
void shutdown_renderer_system(void* state){
    if(state_ptr){
        state_ptr->backend.shutdown(&state_ptr->backend);
    }
    state_ptr = 0;
}

void renderer_on_resize(u16 width, u16 height){
    if(state_ptr){
        state_ptr->backend.resize(&state_ptr->backend, width, height);
    }else{
        PANCAKE_WARN("renderer backend does not exist to accept resize : %i , %i" ,width, height);
    }
}

b8 renderer_begin_frame(f32 delta_time){
    if(!state_ptr) return false;
    return state_ptr->backend.begin_frame(&state_ptr->backend, delta_time);
}
b8 renderer_end_frame(f32 delta_time){
    if(!state_ptr) return false;
    b8 result = state_ptr->backend.end_frame(&state_ptr->backend, delta_time);
    state_ptr->backend.frame_number++;
    return result;
}

b8 renderer_draw_frame(renderer_packet* packet){
    //if begin frame returns true ,the mid-frame operations may continue
    if(renderer_begin_frame(packet->delta_time)){

        //end the frame, if this fails it is likely unrecoverable
        b8 result = renderer_end_frame(packet->delta_time);
        if(!result){
            PANCAKE_FATAL("renderer_end_frame faild, application shutting down...");
            return false;
        }
    }

    return true;
}