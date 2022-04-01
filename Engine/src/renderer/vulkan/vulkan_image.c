#include "vulkan_image.h"
#include "vulkan_device.h"
#include "core/pancake_memory.h"
#include "core/logger.h"

void vulkan_image_create(
    vulkan_context* context,
    VkImageType image_type,
    u32 width,
    u32 height,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags memory_flags,
    b32 create_view,
    VkImageAspectFlags view_aspect_flags,
    vulkan_image* out_image
){
    //copy params
    out_image->width = width;
    out_image->height = height;

    VkImageCreateInfo create_info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    create_info.imageType = VK_IMAGE_TYPE_2D;
    create_info.extent.width = width;
    create_info.extent.height = height;
    create_info.extent.depth = 1;   //TODO: Support configurable depth.
    create_info.mipLevels = 4;      //TODO: Support mip mapping.
    create_info.arrayLayers = 1;    //TODO: Support number of layers in the image.
    create_info.format = format;
    create_info.tiling = tiling;
    create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    create_info.usage = usage;
    create_info.samples = VK_SAMPLE_COUNT_1_BIT;            //TODO: Configurable saple count.
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;    //TODO: Configurable sharing mode.

    VK_CHECK(vkCreateImage(context->device.logical_device, &create_info, context->allocator, &out_image->handle));

    //query memory requirements
    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(context->device.logical_device, out_image->handle, &memory_requirements);

    i32 memory_type = context->find_memory_index(memory_requirements.memoryTypeBits, memory_flags);
    if(memory_type == -1){
        PANCAKE_ERROR("Required memory type not found, Image not valid.");
    }

    //allocate mamory
    VkMemoryAllocateInfo memory_allocate_info = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    memory_allocate_info.allocationSize = memory_requirements.size;
    memory_allocate_info.memoryTypeIndex = memory_type;

    VK_CHECK(vkAllocateMemory(context->device.logical_device, &memory_allocate_info, context->allocator, &out_image->memory));

    //bind the memory
    VK_CHECK(vkBindImageMemory(context->device.logical_device, out_image->handle, out_image->memory, 0));   //TODO: Configurable memory offset

    //create view 
    if(create_view){
        out_image->view = 0;
        vulkan_image_view_create(context, format, out_image, view_aspect_flags);
    }
}

void vulkan_image_view_create(
    vulkan_context* context,
    VkFormat format,
    vulkan_image* image,
    VkImageAspectFlags aspect_flags
){
    VkImageViewCreateInfo create_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    create_info.image = image->handle;
    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;   //TODO: Make Configurable
    create_info.format = format;
    create_info.subresourceRange.aspectMask = aspect_flags;

    //TODO: Make Configurable
    create_info.subresourceRange.baseMipLevel = 0;
    create_info.subresourceRange.levelCount = 1;
    create_info.subresourceRange.baseArrayLayer = 0;
    create_info.subresourceRange.layerCount = 1;

    VK_CHECK(vkCreateImageView(context->device.logical_device, &create_info, context->allocator, &image->view));
}

void vulkan_image_destroy(vulkan_context* context, vulkan_image* image){
    if(image->view){
        vkDestroyImageView(context->device.logical_device, image->view, context->allocator);
        image->view = 0;
    }
    if(image->memory){
        vkFreeMemory(context->device.logical_device, image->memory, context->allocator);
        image->memory = 0;
    }
    if(image->handle){
        vkDestroyImage(context->device.logical_device, image->handle, context->allocator);
        image->handle = 0;
    }
}