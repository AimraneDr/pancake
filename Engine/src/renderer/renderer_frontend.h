#pragma once

#include "renderer_types.inl"

struct static_mesh_data;
struct platform_state;

b8 initialize_renderer_system(u64* memory_requirement, void* state, const char* application_name);
void shutdown_renderer_system(void* state);

void renderer_on_resize(u16 width, u16 height);

b8 renderer_draw_frame(renderer_packet* packet);