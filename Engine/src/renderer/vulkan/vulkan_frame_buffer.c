#include "vulkan_frame_buffer.h"
#include "core/pancake_memory.h"


void vulkan_frame_buffer_create(
    vulkan_context* context,
    vulkan_renderpass* renderpass,
    u32 width,
    u32 height,
    u32 attachments_count,
    VkImageView* attachments,
    vulkan_frame_buffer* out_frame_buffer
){
    //copy to struct
    out_frame_buffer->attachments_count = attachments_count;
    out_frame_buffer->attachments = pancake_allocate(sizeof(VkImageView) * attachments_count, MEMORY_TAG_RENDERER);
    for(u32 i=0; i < attachments_count; ++i){
        out_frame_buffer->attachments[i] = attachments[i];
    }
    out_frame_buffer->renderpass = renderpass;

    //create info
    VkFramebufferCreateInfo create_info = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    create_info.renderPass = renderpass->handle;
    create_info.attachmentCount = attachments_count;
    create_info.pAttachments = out_frame_buffer->attachments;
    create_info.width = width;
    create_info.height = height;
    create_info.layers = 1;

    VK_CHECK(vkCreateFramebuffer(context->device.logical_device, &create_info, context->allocator, &out_frame_buffer->handle));
}

void vulkan_frame_buffer_destroy(vulkan_context* context, vulkan_frame_buffer* frame_buffer){
    vkDestroyFramebuffer(context->device.logical_device, frame_buffer->handle, context->allocator);
    if(frame_buffer->attachments){
        pancake_free(frame_buffer->attachments, sizeof(VkImageView) * frame_buffer->attachments_count, MEMORY_TAG_RENDERER);
        frame_buffer->attachments = 0;
    }
    frame_buffer->handle = 0;
    frame_buffer->attachments_count = 0;
    frame_buffer->renderpass = 0;
}