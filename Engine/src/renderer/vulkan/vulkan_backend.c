#include "vulkan_backend.h"
#include "vulkan_types.inl"
#include "vulkan_device.h"
#include "vulkan_swapchain.h"
#include "vulkan_renderpass.h"
#include "vulkan_command_buffer.h"
#include "vulkan_frame_buffer.h"
#include "vulkan_fence.h"
#include "vulkan_utils.h"
#include "vulkan_buffer.h"
#include "core/logger.h"
#include "core/pancake_memory.h"
#include "core/pancake_string.h"
#include "core/application.h"
#include "containers/list.h"
#include "vulkan_platform.h"
#include "platform/platform.h"
#include "math/math_types.h"

//shaders
#include "shaders/vulkan_object_shader.h"

//static volkan context
static vulkan_context context;
static u32 cached_frame_buffer_width = 0, cached_frame_buffer_height = 0;

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_types,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data) ;

i32 find_memory_index(u32 type_filter, u32 property_flags);

b8 create_buffers(vulkan_context* context);

void create_command_buffers(renderer_backend* backend);
void regenerate_frame_buffers(renderer_backend* backend, vulkan_swapchain* swapchain, vulkan_renderpass* renderpass);

b8 recreate_swapchain(renderer_backend* backend);

void upload_data_range(vulkan_context* context, VkCommandPool pool, VkFence fence, VkQueue queue, vulkan_buffer* buffer, u64 offset, u64 size, void* data) {
    // Create a host-visible staging buffer to upload to. Mark it as the source of the transfer.
    VkBufferUsageFlags flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    vulkan_buffer staging;
    vulkan_buffer_create(context, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, flags, true, &staging);

    // Load the data into the staging buffer.
    vulkan_buffer_load_data(context, &staging, 0, size, 0, data);

    // Perform the copy from staging to the device local buffer.
    vulkan_buffer_copy_to(context, pool, fence, queue, staging.handle, 0, buffer->handle, offset, size);

    // Clean up the staging buffer.
    vulkan_buffer_destroy(context, &staging);
}

b8 vulkan_renderer_backende_initialize(struct renderer_backend* backend, const char* application_name){
    
    //function pointer
    context.find_memory_index = find_memory_index;

    //TODO: costum allocator.
    context.allocator = 0;

    application_get_frame_buffer_size(&cached_frame_buffer_width, &cached_frame_buffer_height);
    context.framebuffer_width = (cached_frame_buffer_width != 0) ? cached_frame_buffer_width : 600;
    context.framebuffer_height = (cached_frame_buffer_height != 0) ? cached_frame_buffer_height : 500;
    cached_frame_buffer_width = 0;
    cached_frame_buffer_height = 0;

    //setup vulkan instance
    VkApplicationInfo app_info = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app_info.apiVersion = VK_API_VERSION_1_2;
    app_info.pApplicationName = application_name;
    app_info.pEngineName = "Pancake Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);

    VkInstanceCreateInfo create_info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    create_info.pApplicationInfo = &app_info;

    //obtain a list of extensions
    const char** required_extensions = list_create(const char*);
    list_push(required_extensions, &VK_KHR_SURFACE_EXTENSION_NAME);     //Geniric surface extension
    platform_get_required_extensions(&required_extensions);             //platform specific extensions

    #if defined(_DEBUG)
    list_push(required_extensions, &VK_EXT_DEBUG_UTILS_EXTENSION_NAME); //debug utilities

    PANCAKE_DEBUG("Required Extensions :");
    u32 length = list_length(required_extensions);
    for(u32 i=0; i < length; ++i){
        PANCAKE_DEBUG(required_extensions[i]);
    }
    #endif

    create_info.enabledExtensionCount = list_length(required_extensions);
    create_info.ppEnabledExtensionNames = required_extensions;


    //Validation Layers
    u32 required_validation_layers_count = 0;
    const char** required_validation_layers_names = 0;

// If validation should be done, get a list of the required validation layert names
// and make sure they exist. Validation layers should only be enabled on non-release builds.
#if defined(_DEBUG)
    PANCAKE_INFO("Validation layers enabled. Enumerating...");

    // The list of validation layers required.
    required_validation_layers_names = list_create(const char*);
    list_push(required_validation_layers_names, &"VK_LAYER_KHRONOS_validation");
    required_validation_layers_count = list_length(required_validation_layers_names);

    // Obtain a list of available validation layers
    u32 available_layer_count = 0;
    VK_CHECK(vkEnumerateInstanceLayerProperties(&available_layer_count, 0));
    VkLayerProperties* available_layers = list_reserve(VkLayerProperties, available_layer_count);
    VK_CHECK(vkEnumerateInstanceLayerProperties(&available_layer_count, available_layers));

    // Verify all required layers are available.
    for (u32 i = 0; i < required_validation_layers_count; ++i) {
        PANCAKE_INFO("Searching for layer: %s...", required_validation_layers_names[i]);
        b8 found = false;
        for (u32 j = 0; j < available_layer_count; ++j) {
            if (strings_equal(required_validation_layers_names[i], available_layers[j].layerName)) {
                found = true;
                PANCAKE_INFO("Found.");
                break;
            }
        }


        if (!found) {
            PANCAKE_FATAL("Required validation layer is missing: %s", required_validation_layers_names[i]);
            return false;
        }
    }
    PANCAKE_INFO("All required validation layers are present.");
#endif
    

// Debugger
#if defined(_DEBUG)
    PANCAKE_DEBUG("Creating Vulkan debugger...");
    u32 log_severity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;  //|
                     //VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

    VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    debug_create_info.messageSeverity = log_severity;
    debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debug_create_info.pfnUserCallback = vk_debug_callback;
#endif

    create_info.enabledLayerCount = required_validation_layers_count;
    create_info.ppEnabledLayerNames = required_validation_layers_names;


    VK_CHECK(vkCreateInstance(&create_info, context.allocator, &context.instance));

#if defined(_DEBUG)
    PFN_vkCreateDebugUtilsMessengerEXT func =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context.instance, "vkCreateDebugUtilsMessengerEXT");
    PANCAKE_ASSERT_MSG(func, "Failed to create debug messenger!");
    VK_CHECK(func(context.instance, &debug_create_info, context.allocator, &context.debug_messenger));
    PANCAKE_DEBUG("Vulkan debugger created.");

    //pass the debug to the instance create info to see debugger output while creating the instance
    create_info.pNext = &debug_create_info;

#endif


    list_destroy(available_layers);
    list_destroy(required_extensions);
    list_destroy(required_validation_layers_names);


    PANCAKE_INFO("Vulkan Instance created.");

    //create surface
    PANCAKE_DEBUG("Create vulkan surface...");
    if(!platform_vulkan_surface_create(&context)){
        PANCAKE_ERROR("Failed to create vulkan surface !");
        return false;
    }
    PANCAKE_DEBUG("Vulkan surface created successfully");

    //create device
    if(!vulkan_device_create(&context)){
        PANCAKE_ERROR("Failed to create device !");
        return false;
    }

    //create Swapchain
    vulkan_swapchain_create(
        &context,
        context.framebuffer_width,
        context.framebuffer_height,
        &context.swapchain
    );

    //create renderpass
    vulkan_renderpass_create(
        &context,
        &context.main_renderpass,
        0, 0, context.framebuffer_width, context.framebuffer_height,
        0.2f, 0.2f, 0.3f, 1.0f,
        1.0f,
        0
    );

    //swapchain framebuffer
    context.swapchain.frame_buffers = list_reserve(vulkan_frame_buffer, context.swapchain.images_count);
    regenerate_frame_buffers(backend, &context.swapchain, &context.main_renderpass);

    //create command_buffers
    create_command_buffers(backend);

    //create sync ojects
    context.image_available_semaphores = list_reserve(VkSemaphore, context.swapchain.max_frames_in_flight);
    context.queue_complete_semaphores = list_reserve(VkSemaphore, context.swapchain.max_frames_in_flight);
    context.in_flight_fence = list_reserve(vulkan_fence, context.swapchain.max_frames_in_flight);

    for(u32 i=0; i < context.swapchain.max_frames_in_flight; ++i){
        VkSemaphoreCreateInfo semaphore_create_info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        vkCreateSemaphore(context.device.logical_device, &semaphore_create_info, context.allocator, &context.image_available_semaphores[i]);
        vkCreateSemaphore(context.device.logical_device, &semaphore_create_info, context.allocator, &context.queue_complete_semaphores[i]);

        //create a fence in a signaled state, indecating thet the first frane has already been "rendered".
        //this will prevent the application from waitin indefinitely for the first frame to render since it cannot be rendered until a frame is rendered before it.
        vulkan_fence_create(&context, true, &context.in_flight_fence[i]);
    }

    //in flight fence should not yet exist at this point, so clear the list.
    //these are stored in pointers because the initial state should be 0, and will be 0 when not in use.
    //actual fence are not owned by this list.
    context.images_in_flight = list_reserve(vulkan_fence, context.swapchain.images_count);
    for(u32 i=0; i < context.swapchain.images_count; ++i){
        context.images_in_flight[i] = 0;
    }

    //Create BuiltIn Shaders
    if(!vulkan_object_shader_create(&context, &context.object_shader)){
        PANCAKE_ERROR("Error occured while loading built-in basic_lighting shader .");
        return false;
    }

    create_buffers(&context);

    // TODO: temporary test code
    const u32 vert_count = 4;
    vertex_3d verts[vert_count];
    pancake_zero_memory(verts, sizeof(vertex_3d) * vert_count);

    verts[0].position.x = 0.0;
    verts[0].position.y = -0.5;

    verts[1].position.x = 0.5;;
    verts[1].position.y = 0.5;;

    verts[2].position.x = 0;
    verts[2].position.y = 0.5;;

    verts[3].position.x = 0.5;;
    verts[3].position.y = -0.5;

    const u32 index_count = 6;
    u32 indices[index_count] = {0, 1, 2, 0, 3, 1};

    upload_data_range(&context, context.device.graphics_command_pool, 0, context.device.graphics_queue, &context.object_vertex_buffer, 0, sizeof(vertex_3d) * vert_count, verts);
    upload_data_range(&context, context.device.graphics_command_pool, 0, context.device.graphics_queue, &context.object_index_buffer, 0, sizeof(u32) * index_count, indices);
    // TODO: end temp code

    PANCAKE_INFO("Vulkan renderer initialized successfully");
    return true;
}

void vulkan_renderer_backende_shutdown(struct renderer_backend* backend){
    vkDeviceWaitIdle(context.device.logical_device);
    //Destroy in the opposit order of creation

    //Destroy buffers
    vulkan_buffer_destroy(&context, &context.object_vertex_buffer);
    vulkan_buffer_destroy(&context, &context.object_index_buffer);

    //destroy shader modules
    vulkan_object_shader_destroy(&context, &context.object_shader);

    //destroy sync objects
    for(u32 i=0; i < context.swapchain.max_frames_in_flight; ++i){
        if(context.image_available_semaphores[i]){
            vkDestroySemaphore(
                context.device.logical_device,
                context.image_available_semaphores[i],
                context.allocator);
            context.image_available_semaphores[i] = 0;
        }
        if(context.queue_complete_semaphores[i]){
            vkDestroySemaphore(
                context.device.logical_device,
                context.queue_complete_semaphores[i],
                context.allocator);
            context.queue_complete_semaphores[i] = 0;
        }
        vulkan_fence_destroy(&context, &context.in_flight_fence[i]);
    }

    list_destroy(context.image_available_semaphores);
    context.image_available_semaphores = 0;

    list_destroy(context.queue_complete_semaphores);
    context.queue_complete_semaphores = 0;
    
    list_destroy(context.in_flight_fence);
    context.in_flight_fence = 0;

    list_destroy(context.images_in_flight);
    context.images_in_flight = 0;



    //destroy command buffers
    for(u32 i=0; i < context.swapchain.images_count; ++i){
        if(context.graphics_command_buffers[i].handle){
            vulkan_command_buffer_free(&context, context.device.graphics_command_pool, &context.graphics_command_buffers[i]);
            context.graphics_command_buffers[i].handle = 0;
        }
    }
    list_destroy(context.graphics_command_buffers);
    context.graphics_command_buffers = 0;

    //destroy frame buffers
    for(u32 i=0; i < context.swapchain.images_count; ++i){
        vulkan_frame_buffer_destroy(&context, &context.swapchain.frame_buffers[i]);
    }

    vulkan_renderpass_destroy(&context, &context.main_renderpass);

    //destroy swapchain
    vulkan_swapchain_destroy(&context, &context.swapchain);
    PANCAKE_DEBUG("vulkan swapchain had benn destroyed successfully");

    PANCAKE_DEBUG("Destroying Vulkan Device...");
    vulkan_device_destroy(&context);
    PANCAKE_DEBUG("vulkan Device had benn destroyed successfully");

    PANCAKE_DEBUG("Destroying Vulkan Surface...");
    if(context.surface){
        vkDestroySurfaceKHR(context.instance, context.surface,context.allocator);
        context.surface = 0;
        PANCAKE_DEBUG("vulkan Surface had benn destroyed successfully");
    }

#if defined(_DEBUG)
    PANCAKE_DEBUG("destroying vulkan debugger...");
    if(context.debug_messenger){
        PFN_vkDestroyDebugUtilsMessengerEXT func =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context.instance, "vkDestroyDebugUtilsMessengerEXT");
        func(context.instance, context.debug_messenger, context.allocator);
        PANCAKE_DEBUG("vulkan debugger had benn destroyed successfully");
    }
#endif

    PANCAKE_DEBUG("destroying vulkan instance...");
    vkDestroyInstance(context.instance, context.allocator);
    PANCAKE_DEBUG("vulkan instance had benn destroyed successfully");

}

void vulkan_renderer_backende_resize(struct renderer_backend* backend, u16 width, u16 height){
    //Update the "Framebuffer size generation". a counterwich indecate when the framebuffer size had been updated
    cached_frame_buffer_width = width;
    cached_frame_buffer_height = height;
    context.frame_buffer_size_generation++;

    PANCAKE_INFO("vulkan_renderer_backend->resized : (width = %i | height = %i | generation = %llu ).", width, height, context.frame_buffer_size_generation);
}

b8 vulkan_renderer_backende_begin_frame(struct renderer_backend* backend, f32 delta_time){
    vulkan_device* device = &context.device;

    //check if recreating_swapchain and boot out
    if(context.recreating_swapchain){
        VkResult result = vkDeviceWaitIdle(device->logical_device);
        if(!vulkan_result_is_success(result)){
            PANCAKE_ERROR("vulkan_renderer_backende_begin_frame vkDeviceWaitIdle (1) failed : '%s'", vulkan_result_string(result, true));
            return false;
        }
        PANCAKE_INFO("recreating swapchain.");
        return false;
    }

    //check if the framebuffer had been resized, if so a new swapchain must be created
    if(context.frame_buffer_size_generation != context.frame_buffer_size_last_generation){
        VkResult result = vkDeviceWaitIdle(device->logical_device);
        if(!vulkan_result_is_success(result)){
            PANCAKE_ERROR("vulkan_renderer_backende_begin_frame vkDeviceWaitIdle (2) failed : '%s'", vulkan_result_string(result, true));
            return false;
        }

        //if swapchain recreation failed (because for example the window is minimized)
        //boot out before unsetting the flag
        if(!recreate_swapchain(backend)){
            return false;
        }

        PANCAKE_INFO("resized, booting out");
        return false;
    }

    //wait for the excution of the current frame to complete. the fence being free will allow this one to move on
    if(!vulkan_fence_wait(&context, &context.in_flight_fence[context.current_frame], UINT64_MAX)){
        PANCAKE_ERROR("in-flight fence wait failure!");
        return false;
    }

    //aquire the next image from the swapchain. Pass along the semaphore that should be signaled when this complete.
    //this same semaphore will later be waited on by the queue submission to insure this image is available
    if(!vulkan_swapchain_acquire_next_image_index(
        &context,
        &context.swapchain,
        UINT64_MAX,
        context.image_available_semaphores[context.current_frame],
        0,
        &context.image_index
    )){
        return false;
    }

    //begin recording commands
    vulkan_command_buffer* command_buffer = &context.graphics_command_buffers[context.image_index];
    vulkan_command_buffer_reset(command_buffer);
    vulkan_command_buffer_begin(command_buffer, false, false, false);

    //dynamic state
    VkViewport view_port;
    view_port.x = 0.0f;
    view_port.y = (f32)context.framebuffer_height;
    view_port.width = (f32)context.framebuffer_width;
    view_port.height = (f32)context.framebuffer_height;
    view_port.minDepth = 0.0f;
    view_port.maxDepth = 1.0f;

    //scissor
    VkRect2D scissor;
    scissor.offset.x = scissor.offset.y = 0;
    scissor.extent.width = context.framebuffer_width;
    scissor.extent.height = context.framebuffer_height;

    vkCmdSetViewport(command_buffer->handle, 0, 1, &view_port);
    vkCmdSetScissor(command_buffer->handle, 0, 1, &scissor);

    context.main_renderpass.w = context.framebuffer_width;
    context.main_renderpass.h = context.framebuffer_height;

    //begin the renderpass
    vulkan_renderpass_begin(command_buffer, &context.main_renderpass, context.swapchain.frame_buffers[context.image_index].handle);

   // TODO: temporary test code
    vulkan_object_shader_use(&context, &context.object_shader);

    // Bind vertex buffer at offset.
    VkDeviceSize offsets[1] = {0};
    vkCmdBindVertexBuffers(command_buffer->handle, 0, 1, &context.object_vertex_buffer.handle, (VkDeviceSize*)offsets);

    // Bind index buffer at offset.
    vkCmdBindIndexBuffer(command_buffer->handle, context.object_index_buffer.handle, 0, VK_INDEX_TYPE_UINT32);

    // Issue the draw.
    vkCmdDrawIndexed(command_buffer->handle, 6, 1, 0, 0, 0);
    // TODO: end temporary test code

    return true;
}


b8 vulkan_renderer_backende_end_frame(struct renderer_backend* backend, f32 delta_time){

    vulkan_command_buffer* command_buffer = &context.graphics_command_buffers[context.image_index];

    //end renderpass
    vulkan_renderpass_end(command_buffer, &context.main_renderpass);

    //end command buffer
    vulkan_command_buffer_end(command_buffer);

    //make sure the previouse frame is not using this image, (i.e. its frame is being waited on)
    if(context.images_in_flight[context.image_index] != VK_NULL_HANDLE){
        vulkan_fence_wait(&context, context.images_in_flight[context.image_index], UINT64_MAX);
    }

    //mark the image fence as in-use by this frame
    context.images_in_flight[context.image_index] = &context.in_flight_fence[context.current_frame];

    //reset the fence for use on the next frame
    vulkan_fence_reset(&context, &context.in_flight_fence[context.current_frame]);

    //submit the queue and wait for the opperation to complete.
    //begin queue submission.
    VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};

    //command buffer(s) to be excuted
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer->handle;

    //the semaphore(s) to be signaled when the queue is complete
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &context.queue_complete_semaphores[context.current_frame];

    //wait semaphore ensures that the opperation cannot begin until the image is available.
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &context.image_available_semaphores[context.current_frame];

    // Each semaphore waits on the corresponding pipeline stage to complete. 1:1 ratio.
    // VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT prevents subsequent colour attachment
    // writes from executing until the semaphore signals (i.e. one frame is presented at a time)
    VkPipelineStageFlags flags[1] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit_info.pWaitDstStageMask = flags;

    VkResult result = vkQueueSubmit(context.device.graphics_queue, 1, &submit_info, context.in_flight_fence[context.current_frame].handle);
    if(result != VK_SUCCESS){
        PANCAKE_ERROR("vkQueueSubmit failed with result : %s", vulkan_result_string(result, true));
        return false;
    }

    vulkan_command_buffer_update_submitted(command_buffer);
    //end queue submission

    //Give the image back to the swapchain
    vulkan_swapchain_present(
        &context,
        &context.swapchain,
        context.device.graphics_queue,
        context.device.present_queue,
        context.queue_complete_semaphores[context.current_frame],
        context.image_index
    );

    return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_types,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data) {
    switch (message_severity) {
        default:
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            PANCAKE_ERROR(callback_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            PANCAKE_WARN(callback_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            PANCAKE_INFO(callback_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            PANCAKE_TRACE(callback_data->pMessage);
            break;
    }
    return VK_FALSE;
}

i32 find_memory_index(u32 type_filter, u32 property_flags){
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(context.device.physical_device, &memory_properties);

    for(u32 i=0; i < memory_properties.memoryTypeCount; ++i){
        //check each memory type to see if the bit is set to 1
        if(type_filter & (1 << i) && (memory_properties.memoryTypes[i].propertyFlags & property_flags) == property_flags){
            return 1;
        }
    }

    PANCAKE_WARN("Unable to find a suitable memory type !");
    return -1;
}

void create_command_buffers(renderer_backend* backend){
    if(!context.graphics_command_buffers){
        context.graphics_command_buffers = list_reserve(vulkan_command_buffer, context.swapchain.images_count);
        for(u32 i=0; i < context.swapchain.images_count; ++i){
            pancake_zero_memory(&context.graphics_command_buffers[i], sizeof(vulkan_command_buffer));
        }
    }

    for(u32 i=0; i < context.swapchain.images_count; ++i){
        if(context.graphics_command_buffers[i].handle){
            vulkan_command_buffer_free(&context, context.device.graphics_command_pool, &context.graphics_command_buffers[i]);
        }
        pancake_zero_memory(&context.graphics_command_buffers[i], sizeof(VkCommandBuffer));
        vulkan_command_buffer_allocate(&context, context.device.graphics_command_pool, true, &context.graphics_command_buffers[i]);
    }

    PANCAKE_INFO("Vulkan Command Buffer(s) had been craeted successfully.");
}

void regenerate_frame_buffers(renderer_backend* backend, vulkan_swapchain* swapchain, vulkan_renderpass* renderpass){
    for(u32 i=0; i < swapchain->images_count; ++i){
        //TODO: Make this dynamic based on the currentconfigured attachments
        u32 attachments_count = 2;
        VkImageView attachments[] = {
            swapchain->views[i],
            swapchain->depth_attachment.view
        };

        vulkan_frame_buffer_create(
            &context,
            renderpass,
            context.framebuffer_width,
            context.framebuffer_height,
            attachments_count,
            attachments,
            &swapchain->frame_buffers[i]
        );
    }
}

b8 recreate_swapchain(renderer_backend* backend){
    // If already being recreated, do not try again.
    if (context.recreating_swapchain) {
        PANCAKE_DEBUG("recreate_swapchain called when already recreating. Booting.");
        return false;
    }

    // Detect if the window is too small to be drawn to
    if (context.framebuffer_width == 0 || context.framebuffer_height == 0) {
        PANCAKE_DEBUG("recreate_swapchain called when window is < 1 in a dimension. Booting.");
        return false;
    }

    // Mark as recreating if the dimensions are valid.
    context.recreating_swapchain = true;

    // Wait for any operations to complete.
    vkDeviceWaitIdle(context.device.logical_device);

    // Clear these out just in case.
    for (u32 i = 0; i < context.swapchain.images_count; ++i) {
        context.images_in_flight[i] = 0;
    }

    // Requery support
    vulkan_device_query_swapchain_support(
        context.device.physical_device,
        context.surface,
        &context.device.swapchain_support);
    vulkan_device_detect_depth_format(&context.device);

    vulkan_swapchain_recreate(
        &context,
        cached_frame_buffer_width,
        cached_frame_buffer_height,
        &context.swapchain);

    // Sync the framebuffer size with the cached sizes.
    context.framebuffer_width = cached_frame_buffer_width;
    context.framebuffer_height = cached_frame_buffer_height;
    context.main_renderpass.w = context.framebuffer_width;
    context.main_renderpass.h = context.framebuffer_height;
    cached_frame_buffer_width = 0;
    cached_frame_buffer_height = 0;

    // Update framebuffer size generation.
    context.frame_buffer_size_last_generation = context.frame_buffer_size_generation;

    // cleanup swapchain
    for (u32 i = 0; i < context.swapchain.images_count; ++i) {
        vulkan_command_buffer_free(&context, context.device.graphics_command_pool, &context.graphics_command_buffers[i]);
    }

    // Framebuffers.
    for (u32 i = 0; i < context.swapchain.images_count; ++i) {
        vulkan_frame_buffer_destroy(&context, &context.swapchain.frame_buffers[i]);
    }

    context.main_renderpass.x = 0;
    context.main_renderpass.y = 0;
    context.main_renderpass.w = context.framebuffer_width;
    context.main_renderpass.h = context.framebuffer_height;

    regenerate_frame_buffers(backend, &context.swapchain, &context.main_renderpass);

    create_command_buffers(backend);

    // Clear the recreating flag.
    context.recreating_swapchain = false;

    return true;
}

b8 create_buffers(vulkan_context* context){
    VkMemoryPropertyFlagBits memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    const u64 vertex_buffer_size = sizeof(vertex_3d) * 1024 * 1024;
    if (!vulkan_buffer_create(
            context,
            vertex_buffer_size,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            memory_property_flags,
            true,
            &context->object_vertex_buffer)) {
        PANCAKE_ERROR("Error creating vertex buffer.");
        return false;
    }
    context->geometry_vertex_offset = 0;

    const u64 index_buffer_size = sizeof(u32) * 1024 * 1024;
    if (!vulkan_buffer_create(
            context,
            index_buffer_size,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            memory_property_flags,
            true,
            &context->object_index_buffer)) {
        PANCAKE_ERROR("Error creating vertex buffer.");
        return false;
    }
    context->geometry_index_offset = 0;

    return true;
}