#pragma once

#include "defines.h"

typedef enum renderer_backend_types{
    RENDERER_BACKEND_TYPE_VULKAN,
    RENDERER_BACKEND_TYPE_OPENGL,
    RENDERER_BACKEND_TYPE_DIRECTX,
    RENDERER_BACKEND_TYPE_METAL
}renderer_backend_types;

typedef struct renderer_backend{
    u64 frame_number;

    b8 (*initialize)(struct renderer_backend* backend, const char* application_name);
    void (*shutdown)(struct renderer_backend* backend);
    void (*resize)(struct renderer_backend* backend, u16 width, u16 height);
    b8 (*begin_frame)(struct renderer_backend* backend, f32 delta_time);
    b8 (*end_frame)(struct renderer_backend* backend, f32 delta_time);
}renderer_backend;

typedef struct renderer_packet{
    f32 delta_time;
}renderer_packet;