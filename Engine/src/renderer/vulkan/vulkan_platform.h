#pragma once

#include "defines.h"

struct platform_state;
struct vulkan_context;

b8 platform_vulkan_surface_create(struct vulkan_context* context);

/**
 * Appends the names of required extensions for this platform to
 * the names_list, which should be created and passed in.
 */
void platform_get_required_extensions(const char*** names_list);