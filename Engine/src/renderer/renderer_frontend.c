#include "renderer_frontend.h"
#include "renderer_backend.h"
#include "core/logger.h"
#include "core/pancake_memory.h"

//backend render context
static renderer_backend* backend = 0;


b8 initialize_renderer(const char* application_name, struct platform_state* plat_state){
    backend = pancake_allocate(sizeof(renderer_backend), MEMORY_TAG_RENDERER);

    //TODO: make this configurable
    renderer_backend_create(RENDERER_BACKEND_TYPE_VULKAN, plat_state, backend);
    backend->frame_number = 0;

    if(!backend->initialize(backend,application_name,plat_state)){
        PANCAKE_FATAL("Rendeerer backend failed to initialize , shutting down...");
        return false;
    }


    return true;
}
void shutdown_renderer(){
    backend->shutdown(backend);
    pancake_free(backend, sizeof(renderer_backend), MEMORY_TAG_RENDERER);
}

void renderer_on_resize(u16 width, u16 height){
    if(backend){
        backend->resize(backend, width, height);
    }else{
        PANCAKE_WARN("renderer backend does not exist to accept resize : %i , %i" ,width, height);
    }
}

b8 renderer_begin_frame(f32 delta_time){
    return backend->begin_frame(backend, delta_time);
}
b8 renderer_end_frame(f32 delta_time){
    b8 result = backend->end_frame(backend, delta_time);
    backend->frame_number++;
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