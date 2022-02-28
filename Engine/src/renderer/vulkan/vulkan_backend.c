#include "vulkan_backend.h"
#include "vulkan_types.inl"
#include "vulkan_device.h"
#include "vulkan_swapchain.h"
#include "vulkan_renderpass.h"
#include "core/logger.h"
#include "core/pancake_string.h"
#include "containers/list.h"
#include "vulkan_platform.h"
#include "platform/platform.h"

//static volkan context
static vulkan_context context;

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_types,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data) ;

i32 find_memory_index(u32 type_filter, u32 property_flags);

b8 vulkan_renderer_backende_initialize(struct renderer_backend* backend, const char* application_name, struct platform_state* plat_state){
    
    //function pointer
    context.find_memory_index = find_memory_index;

    //TODO: costum allocator.
    context.allocator = 0;
    
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
        b8 found = FALSE;
        for (u32 j = 0; j < available_layer_count; ++j) {
            if (strings_equal(required_validation_layers_names[i], available_layers[j].layerName)) {
                found = TRUE;
                PANCAKE_INFO("Found.");
                break;
            }
        }


        if (!found) {
            PANCAKE_FATAL("Required validation layer is missing: %s", required_validation_layers_names[i]);
            return FALSE;
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
    if(!platform_vulkan_surface_create(plat_state, &context)){
        PANCAKE_ERROR("Failed to create vulkan surface !");
        return FALSE;
    }
    PANCAKE_DEBUG("Vulkan surface created successfully");

    //create device
    if(!vulkan_device_create(&context)){
        PANCAKE_ERROR("Failed to create device !");
        return FALSE;
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
        0.0f, 0.0f, 0.2f, 1.0f,
        1.0f,
        0
    );

    PANCAKE_INFO("Vulkan renderer initialized successfully");
    return TRUE;
}
void vulkan_renderer_backende_shutdown(struct renderer_backend* backend){
    //Destroy in the opposit order of creation

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

}

b8 vulkan_renderer_backende_begin_frame(struct renderer_backend* backend, f32 delta_time){
    return TRUE;
}
b8 vulkan_renderer_backende_end_frame(struct renderer_backend* backend, f32 delta_time){
    return TRUE;
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