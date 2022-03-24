#include "vulkan_fence.h"
#include "core/logger.h"

void vulkan_fence_create(vulkan_context* context, b8 create_signaled, vulkan_fence* out_fence){
    //make sure to signaled the fence if required
    out_fence->is_signaled = create_signaled;

    VkFenceCreateInfo create_info = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    if(out_fence->is_signaled){
        create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    }

    vkCreateFence(context->device.logical_device, &create_info, context->allocator, &out_fence->handle);
}

void vulkan_fence_destroy(vulkan_context* context, vulkan_fence* fence){
    if(fence->handle){
        vkDestroyFence(context->device.logical_device, fence->handle, context->allocator);
        fence->handle = 0;
    }
    fence->is_signaled = false;
}

b8 vulkan_fence_wait(vulkan_context* context, vulkan_fence* fence, u64 timed_out){
    if(!fence->is_signaled){
        VkResult result = vkWaitForFences(context->device.logical_device, 1, &fence->handle, true, timed_out);
        switch (result)
        {
        case VK_SUCCESS:
            fence->is_signaled = true;
            return true;
        case VK_TIMEOUT:
            PANCAKE_WARN("vk_fence_wait - time out !");
            break;
        case VK_ERROR_DEVICE_LOST:
            PANCAKE_ERROR("vk_fence_wait - VK_ERROR_DEVICE_LOST");
            break;
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            PANCAKE_ERROR("vk_fence_wait - VK_ERROR_OUT_OF_HOST_MEMORY");
            break;
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            PANCAKE_ERROR("vk_fence_wait - VK_ERROR_OUT_OF_DEVICE_MEMORY");
            break;
        default:
            PANCAKE_ERROR("vk_fence_wait - An unkown error has occurred");
            break;
        }
    }else{
        //if already signaled don't wait
        return true;
    }

    return false;
}

void vulkan_fence_reset(vulkan_context* context, vulkan_fence* fence){
    if(fence->is_signaled){
        VK_CHECK(vkResetFences(context->device.logical_device, 1, &fence->handle));
        fence->is_signaled = false;
    }
}