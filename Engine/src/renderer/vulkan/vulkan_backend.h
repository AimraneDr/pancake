#pragma once

#include "renderer/renderer_backend.h"

b8 vulkan_renderer_backende_initialize(struct renderer_backend* backend, const char* application_name);
void vulkan_renderer_backende_shutdown(struct renderer_backend* backend);

void vulkan_renderer_backende_resize(struct renderer_backend* backend, u16 width, u16 height);

b8 vulkan_renderer_backende_begin_frame(struct renderer_backend* backend, f32 delta_time);
b8 vulkan_renderer_backende_end_frame(struct renderer_backend* backend, f32 delta_time);