#include "renderer_backend.h"
#include "vulkan/vulkan_backend.h"

b8 renderer_backend_create(renderer_backend_types type, struct platform_state* plat_state, renderer_backend* out_renderer_backend){
    out_renderer_backend->plat_state = plat_state;

    if(type == RENDERER_BACKEND_TYPE_VULKAN){
        out_renderer_backend->initialize = vulkan_renderer_backende_initialize;
        out_renderer_backend->shutdown = vulkan_renderer_backende_shutdown;
        out_renderer_backend->begin_frame = vulkan_renderer_backende_begin_frame;
        out_renderer_backend->end_frame = vulkan_renderer_backende_end_frame;
        out_renderer_backend->resize = vulkan_renderer_backende_resize;

        return true;
    }

    return false;
}

void renderer_backend_destroy(renderer_backend* renderer_backend){
    renderer_backend->begin_frame = 0;
    renderer_backend->initialize = 0;
    renderer_backend->plat_state = 0;
    renderer_backend->resize = 0;
    renderer_backend->shutdown = 0;
}