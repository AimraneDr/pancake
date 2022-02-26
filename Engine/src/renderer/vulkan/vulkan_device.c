#include "vulkan_device.h"
#include "core/logger.h"
#include "core/pancake_string.h"
#include "core/pancake_memory.h"
#include "containers/list.h"

typedef struct vulkan_physical_device_requirements{
    b8 graphics;
    b8 present;
    b8 compute;
    b8 transfer;
    //list
    const char** device_extensions_names;
    b8 sampler_anisotropy;
    b8 discrete_gpu;
} vulkan_physical_device_requirements;

typedef struct vulkan_physical_device_queue_family_info{
    u32 graphics_family_index;
    u32 present_family_index;
    u32 compute_family_index;
    u32 transfer_family_index;
}vulkan_physical_device_queue_family_info;

b8 select_physical_device(vulkan_context* context);
b8 physical_device_meets_requirements(
    VkPhysicalDevice device, 
    VkSurfaceKHR surface, 
    const VkPhysicalDeviceProperties* properties, 
    const VkPhysicalDeviceFeatures* features,
    const vulkan_physical_device_requirements* requirements,
    vulkan_physical_device_queue_family_info* out_queue_family_info,
    vulkan_swapchain_support_info* out_swapchain_info);

b8 vulkan_device_create(vulkan_context* context){
    
    if(!select_physical_device(context)) return FALSE;

    PANCAKE_INFO("Creating Logical Device...");
    //NOTE: do not create additional queues for shared indices
    b8 present_shared_graphics_queue = context->device.graphics_queue_index == context->device.present_queue_index;
    b8 transfer_shared_graphics_queue = context->device.graphics_queue_index == context->device.transfer_queue_index;
    u32 index_count = 1;

    if(!present_shared_graphics_queue){
        index_count++;
    }
     if(!transfer_shared_graphics_queue){
        index_count++;
    }

    u32 indeces[index_count];
    u8 index = 0;
    indeces[index++] = context->device.graphics_queue_index;
    if(!present_shared_graphics_queue){
        indeces[index++] = context->device.present_queue_index;
    }
     if(!transfer_shared_graphics_queue){
        indeces[index++] = context->device.transfer_queue_index;
    }

    VkDeviceQueueCreateInfo queue_create_info_list[index_count];
    for(u32 i=0; i < index_count; ++i){
        queue_create_info_list[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info_list[i].queueFamilyIndex = indeces[i];
        queue_create_info_list[i].queueCount = 1;
        //TODO: Enable this when needed and fix it
        // if(indeces[i] == context->device.graphics_queue_index){
        //     queue_create_info_list[i].queueCount = 2;
        // }
        queue_create_info_list[i].flags = 0;
        queue_create_info_list[i].pNext = 0;
        f32 queue_priority = 1.0f;
        queue_create_info_list[i].pQueuePriorities = &queue_priority;
    }

    //request device features
    //TODO: should be config driven
    VkPhysicalDeviceFeatures device_features = {};
    device_features.samplerAnisotropy = VK_TRUE;    //request Anisotropy 

    VkDeviceCreateInfo device_create_info = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    device_create_info.queueCreateInfoCount = index_count;
    device_create_info.pQueueCreateInfos = queue_create_info_list;
    device_create_info.pEnabledFeatures = &device_features;
    device_create_info.enabledExtensionCount = 1;
    const char* extension_name = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    device_create_info.ppEnabledExtensionNames = &extension_name;

    // Deprecated and ignored, so pass nothing.
    device_create_info.enabledLayerCount = 0;
    device_create_info.ppEnabledLayerNames = 0;

    //create the device
    VK_CHECK(vkCreateDevice(
                            context->device.physical_device,
                            &device_create_info,
                            context->allocator,
                            &context->device.logical_device
    ));

    PANCAKE_INFO("Logical Device Created Successfully.");

    //GetQueues
    vkGetDeviceQueue(
        context->device.logical_device,
        context->device.graphics_queue_index,
        0,
        &context->device.graphics_queue
    );
    vkGetDeviceQueue(
        context->device.logical_device,
        context->device.present_queue_index,
        0,
        &context->device.present_queue
    );
    vkGetDeviceQueue(
        context->device.logical_device,
        context->device.transfer_queue_index,
        0,
        &context->device.transfer_queue
    );
    vkGetDeviceQueue(
        context->device.logical_device,
        context->device.compute_queue_index,
        0,
        &context->device.compute_queue
    );
    PANCAKE_INFO("Queues Obtained.")


    return TRUE;
}

void vulkan_device_destroy(vulkan_context* context){

    context->device.graphics_queue = 0;
    context->device.present_queue = 0;
    context->device.transfer_queue = 0;
    context->device.compute_queue = 0;

    //destroy the logicsl device
    PANCAKE_INFO("Destroying Logical Device...");
    if(context->device.logical_device){
        vkDestroyDevice(context->device.logical_device, context->allocator);
        context->device.logical_device = 0;
        PANCAKE_INFO("Logical Device Destroyed");
    }


    //physical devices are not destroyable
    PANCAKE_INFO("Releasing physical devaice resources...");
    context->device.physical_device = 0;

    if(context->device.swapchain_support.formats){
        pancake_free(
            context->device.swapchain_support.formats,
            sizeof(VkSurfaceFormatKHR) * context->device.swapchain_support.format_count,
            MEMORY_TAG_RENDERER
        );
        context->device.swapchain_support.formats = 0;
        context->device.swapchain_support.format_count = 0;
    }

    if(context->device.swapchain_support.present_modes){
        pancake_free(
            context->device.swapchain_support.present_modes,
            sizeof(VkPresentModeKHR) * context->device.swapchain_support.present_mode_count,
            MEMORY_TAG_RENDERER
        );
        context->device.swapchain_support.present_modes = 0;
        context->device.swapchain_support.present_mode_count = 0;
    }

    pancake_zero_memory(
        &context->device.swapchain_support.capabilities,
        sizeof(context->device.swapchain_support.capabilities)
    );

    context->device.graphics_queue_index = -1;
    context->device.present_queue_index = -1;
    context->device.transfer_queue_index = -1;
    context->device.compute_queue_index = -1;
}

void vulkan_device_query_swapchain_support(VkPhysicalDevice physical_device, VkSurfaceKHR surface, vulkan_swapchain_support_info* out_swapchain_info){
    //surface capabilities
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &out_swapchain_info->capabilities));

    //surface format
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &out_swapchain_info->format_count, 0));
    if(out_swapchain_info->format_count != 0){
        if(!out_swapchain_info->formats){
            out_swapchain_info->formats = pancake_allocate(sizeof(VkSurfaceFormatKHR) * out_swapchain_info->format_count, MEMORY_TAG_RENDERER);
        }
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &out_swapchain_info->format_count, out_swapchain_info->formats));
    }

    //surface present
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &out_swapchain_info->present_mode_count, 0));
    if(out_swapchain_info->present_mode_count != 0){
        if(!out_swapchain_info->present_modes){
            out_swapchain_info->present_modes = pancake_allocate(sizeof(VkPresentModeKHR) * out_swapchain_info->present_mode_count, MEMORY_TAG_RENDERER);
        }
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &out_swapchain_info->present_mode_count, out_swapchain_info->present_modes));
    }

}

b8 select_physical_device(vulkan_context* context){
    PANCAKE_INFO("Selecting Physical Device...");
    u32 physical_devices_count = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(context->instance, &physical_devices_count, 0));
    if(physical_devices_count == 0){
        PANCAKE_FATAL("No device wich support Vulkan Were found !");
        return FALSE;
    }

    VkPhysicalDevice physical_devices[physical_devices_count];
    VK_CHECK(vkEnumeratePhysicalDevices(context->instance, &physical_devices_count, physical_devices));

    for(u32 i=0; i < physical_devices_count; ++i){
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physical_devices[i], &properties);

        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(physical_devices[i], &features);

        VkPhysicalDeviceMemoryProperties memory_props;
        vkGetPhysicalDeviceMemoryProperties(physical_devices[i], &memory_props);

        //TODO: this requirement should be driven by the engine configuration
        vulkan_physical_device_requirements requirements = {};
        requirements.graphics = TRUE;
        requirements.present = TRUE;
        requirements.transfer = TRUE;
        //NOTE: enable compute when it is required
        requirements.compute = TRUE;
        requirements.sampler_anisotropy = TRUE;
        //requirements.discrete_gpu = TRUE;
        requirements.device_extensions_names = list_create(const char*);
        list_push(requirements.device_extensions_names, &VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        vulkan_physical_device_queue_family_info queue_info = {};
        b8 result = physical_device_meets_requirements(
                                                        physical_devices[i],
                                                        context->surface,
                                                        &properties,
                                                        &features,
                                                        &requirements,
                                                        &queue_info,
                                                        &context->device.swapchain_support
                                                        );

        if(result){
            PANCAKE_INFO("Selected Device : %s", properties.deviceName);
            //GPU type
            switch(properties.deviceType){
                default:
                case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                    PANCAKE_INFO("GPU type is Unkown.");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                    PANCAKE_INFO("GPU type is integrated.");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                    PANCAKE_INFO("GPU type is discrete.");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                    PANCAKE_INFO("GPU type is virtual.");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_CPU:
                    PANCAKE_INFO("GPU type is cpu.");
                    break;
            }

            //device version
            PANCAKE_INFO(
                        "GPU Device Version : %d.%d.%d",
                        VK_VERSION_MAJOR(properties.driverVersion),
                        VK_VERSION_MINOR(properties.driverVersion),
                        VK_VERSION_PATCH(properties.driverVersion));

            //vulkan api version
            PANCAKE_INFO(
                        "Vulkan API Version : %d.%d.%d",
                        VK_VERSION_MAJOR(properties.apiVersion),
                        VK_VERSION_MINOR(properties.apiVersion),
                        VK_VERSION_PATCH(properties.apiVersion));

            //memory info
            for(u32 j=0; j < memory_props.memoryHeapCount; ++j){
                f32 memory_size_gb = (((f32)memory_props.memoryHeaps[j].size) / 1024.0f / 1024.0f / 1024.0f);
                if(memory_props.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT){
                    PANCAKE_INFO("local GPU memory : %.2f Gb", memory_size_gb);
                } else {
                    PANCAKE_INFO("shared System memory : %.2f Gb", memory_size_gb);
                }
            }

            context->device.physical_device = physical_devices[i];
            context->device.graphics_queue_index = queue_info.graphics_family_index;
            context->device.present_queue_index = queue_info.present_family_index;
            context->device.transfer_queue_index = queue_info.transfer_family_index;
            //NOTE: set comput index here if needed.

            //keep a copy of properties, features and memory info for a later use.
            context->device.properties = properties;
            context->device.featurs = features;
            context->device.memory = memory_props;

            break;
        }
    }
    //Ensure a device was selected
    if(!context->device.physical_device){
        PANCAKE_ERROR("No Devices Wich Meets Requirements Wrer Found");
        return FALSE;
    }

    PANCAKE_INFO("Physical Devise Is Selected.");
    return TRUE;
}

b8 physical_device_meets_requirements(
    VkPhysicalDevice device, 
    VkSurfaceKHR surface, 
    const VkPhysicalDeviceProperties* properties, 
    const VkPhysicalDeviceFeatures* features,
    const vulkan_physical_device_requirements* requirements,
    vulkan_physical_device_queue_family_info* out_queue_family_info,
    vulkan_swapchain_support_info* out_swapchain_info){
    
    //evaluate device properties to determine if it meets the requirements
    out_queue_family_info->graphics_family_index = -1;
    out_queue_family_info->present_family_index = -1;
    out_queue_family_info->compute_family_index = -1;
    out_queue_family_info->transfer_family_index = -1;

    //Discrete GPU ?
    if(requirements->discrete_gpu){
        if(properties->deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU){
            PANCAKE_INFO("Device is not a discrete GPU, Discrete GPU is required. Skipping.");
            return FALSE;
        }
    }

    u32 queue_families_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_families_count, 0);
    VkQueueFamilyProperties queue_families[queue_families_count];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_families_count, queue_families);

    //look at each queue and see what it supports
    PANCAKE_INFO("Graphics | Present | Compute | Transfer | Name");
    u8 min_transfer_score = 255;
    for(u32 i=0; i < queue_families_count; ++i){
        u8 current_transfer_Score = 0;

        if(queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT){
            out_queue_family_info->graphics_family_index = i;
            ++current_transfer_Score;
        }

        if(queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT){
            out_queue_family_info->compute_family_index = i;
            ++current_transfer_Score;
        }

        if(queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT){
            //take the index if it is the current lowest, this increases the liklihood that it is a dedicated transfer queue
            if(current_transfer_Score <= min_transfer_score){
                min_transfer_score = current_transfer_Score;
                out_queue_family_info->transfer_family_index = i;
            }
        }

        //present queue ?
        VkBool32 support_present = VK_FALSE;
        VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &support_present));
        if(support_present){
            out_queue_family_info->present_family_index = i;
        }
    }

    //print out some info about the device
    PANCAKE_INFO("       %d |       %d |       %d |        %d | %s",
                    out_queue_family_info->graphics_family_index != -1,
                    out_queue_family_info->present_family_index != -1,
                    out_queue_family_info->compute_family_index != -1,
                    out_queue_family_info->transfer_family_index != -1,
                    properties->deviceName);

    if(
        (!requirements->graphics || (requirements->graphics && out_queue_family_info->graphics_family_index != -1)) &&
        (!requirements->present || (requirements->present && out_queue_family_info->present_family_index != -1)) &&
        (!requirements->compute || (requirements->compute && out_queue_family_info->compute_family_index != -1)) &&
        (!requirements->transfer || (requirements->transfer && out_queue_family_info->transfer_family_index != -1))
    ){
        PANCAKE_INFO("device meets queue requirements.");
        PANCAKE_TRACE("graphics family index: %i", out_queue_family_info->graphics_family_index);
        PANCAKE_TRACE("present family index: %i", out_queue_family_info->present_family_index);
        PANCAKE_TRACE("compute family index: %i", out_queue_family_info->compute_family_index);
        PANCAKE_TRACE("transfer family index: %i", out_queue_family_info->transfer_family_index);

        //query swap chain support
        vulkan_device_query_swapchain_support(device, surface, out_swapchain_info);

        if(out_swapchain_info->format_count < 1 || out_swapchain_info->present_mode_count < 1){
            if(out_swapchain_info->formats){
                pancake_free(out_swapchain_info->formats, sizeof(VkSurfaceFormatKHR) * out_swapchain_info->format_count, MEMORY_TAG_RENDERER);
            }

            if(out_swapchain_info->present_modes){
                pancake_free(out_swapchain_info->present_modes, sizeof(VkPresentModeKHR) * out_swapchain_info->present_mode_count, MEMORY_TAG_RENDERER);
            }

            PANCAKE_INFO("Required swapchain support not present, skipping device.");
            return FALSE;
        }

        //device extensions
        if(requirements->device_extensions_names){
            u32 available_extensions_count = 0;
            VkExtensionProperties* available_extensions = 0;
            VK_CHECK(vkEnumerateDeviceExtensionProperties(device, 0, &available_extensions_count, 0));
            if(available_extensions_count != 0){
                available_extensions = pancake_allocate(sizeof(VkExtensionProperties) * available_extensions_count, MEMORY_TAG_RENDERER);
                VK_CHECK(vkEnumerateDeviceExtensionProperties(device, 0, &available_extensions_count, available_extensions));
                
                u32 required_extensions_count = list_length(requirements->device_extensions_names);
                for(u32 i=0; i < required_extensions_count; ++i){
                    b8 found = FALSE;

                    for(u32 j=0; j < available_extensions_count; ++j){
                        if(strings_equal(requirements->device_extensions_names[i], available_extensions[j].extensionName)){
                            found = TRUE;
                            break;
                        }
                    }

                    if(!found){
                        PANCAKE_INFO("required extension ' %s ' not found, skipping device", requirements->device_extensions_names[i]);
                        pancake_free(available_extensions, sizeof(VkExtensionProperties) * available_extensions_count, MEMORY_TAG_RENDERER);
                        return FALSE;
                    }
                }
            }
            pancake_free(available_extensions, sizeof(VkExtensionProperties) * available_extensions_count, MEMORY_TAG_RENDERER);
        }

        //sampler anisotropy
        if(requirements->sampler_anisotropy && !features->samplerAnisotropy){
            PANCAKE_INFO("device does not support SamplerAnisotropy, skipping device.");
            return FALSE;
        }


        //device meets all requirements
        return TRUE;
    }

    return FALSE;
}
