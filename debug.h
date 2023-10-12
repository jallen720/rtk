/// Macros
////////////////////////////////////////////////////////////
#define RTK_VK_RESULT_NAME(VK_RESULT) VK_RESULT, #VK_RESULT
#define RTK_ENUM_NAME_CASE(ENUM) case ENUM: return #ENUM;
#define RTK_CHECK_PRINT_FLAG(VAR, FLAG) \
    if ((VAR & FLAG) == FLAG) \
    { \
        PrintTabs(tabs); \
        PrintLine(#FLAG); \
    }

/// Data
////////////////////////////////////////////////////////////
using DebugCallback = VKAPI_ATTR VkBool32 (VKAPI_CALL*)(VkDebugUtilsMessageSeverityFlagBitsEXT msg_severity_flag_bit,
                                                        VkDebugUtilsMessageTypeFlagsEXT msg_type_flags,
                                                        const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
                                                        void* user_data);

struct VkResultInfo
{
    VkResult    result;
    const char* name;
    const char* message;
};

/// Utils
////////////////////////////////////////////////////////////
static uint32 PrintToChar(const char* msg, uint32 msg_size, char end_char, uint32 index, const char* color = NULL)
{
    uint32 start = index;
    while (index < msg_size)
    {
        char curr_char = msg[index];
        if (curr_char == end_char)
        {
            break;
        }
        ++index;
    }

    if (color != NULL)
    {
        Print("%s%.*s" CTK_ANSI_RESET, color, index - start, msg + start);
    }
    else
    {
        Print("%.*s" CTK_ANSI_RESET, index - start, msg + start);
    }

    return index;
}

/// Interface
////////////////////////////////////////////////////////////
static void PrintResult(VkResult result)
{
    static constexpr VkResultInfo VK_RESULT_DEBUG_INFOS[] =
    {
        {
            RTK_VK_RESULT_NAME(VK_SUCCESS),
            "Command successfully completed."
        },
        {
            RTK_VK_RESULT_NAME(VK_NOT_READY),
            "A fence or query has not yet completed."
        },
        {
            RTK_VK_RESULT_NAME(VK_TIMEOUT),
            "A wait operation has not completed in the specified time."
        },
        {
            RTK_VK_RESULT_NAME(VK_EVENT_SET),
            "An event is signaled."
        },
        {
            RTK_VK_RESULT_NAME(VK_EVENT_RESET),
            "An event is unsignaled."
        },
        {
            RTK_VK_RESULT_NAME(VK_INCOMPLETE),
            "A return array was too small for the result."
        },
        {
            RTK_VK_RESULT_NAME(VK_SUBOPTIMAL_KHR),
            "A swapchain no longer matches the surface properties exactly, but can still be used to present to the "
            "surface successfully."
        },
        {
            RTK_VK_RESULT_NAME(VK_ERROR_OUT_OF_HOST_MEMORY),
            "A host memory allocation has failed."
        },
        {
            RTK_VK_RESULT_NAME(VK_ERROR_OUT_OF_DEVICE_MEMORY),
            "A device memory allocation has failed."
        },
        {
            RTK_VK_RESULT_NAME(VK_ERROR_INITIALIZATION_FAILED),
            "Initialization of an object could not be completed for implementation-specific reasons."
        },
        {
            RTK_VK_RESULT_NAME(VK_ERROR_DEVICE_LOST),
            "The logical or physical device has been lost."
        },
        {
            RTK_VK_RESULT_NAME(VK_ERROR_MEMORY_MAP_FAILED),
            "Mapping of a memory object has failed."
        },
        {
            RTK_VK_RESULT_NAME(VK_ERROR_LAYER_NOT_PRESENT),
            "A requested layer is not present or could not be loaded."
        },
        {
            RTK_VK_RESULT_NAME(VK_ERROR_EXTENSION_NOT_PRESENT),
            "A requested extension is not supported."
        },
        {
            RTK_VK_RESULT_NAME(VK_ERROR_FEATURE_NOT_PRESENT),
            "A requested feature is not supported."
        },
        {
            RTK_VK_RESULT_NAME(VK_ERROR_INCOMPATIBLE_DRIVER),
            "The requested version of Vulkan is not supported by the driver or is otherwise incompatible for "
            "implementation-specific reasons."
        },
        {
            RTK_VK_RESULT_NAME(VK_ERROR_TOO_MANY_OBJECTS),
            "Too many objects of the type have already been created."
        },
        {
            RTK_VK_RESULT_NAME(VK_ERROR_FORMAT_NOT_SUPPORTED),
            "A requested format is not supported on this device."
        },
        {
            RTK_VK_RESULT_NAME(VK_ERROR_FRAGMENTED_POOL),
            "A pool allocation has failed due to fragmentation of the pool’s memory. This must only be returned if no "
            "attempt to allocate host or device memory was made to accommodate the new allocation. This should be "
            "returned in preference to VK_ERROR_OUT_OF_POOL_MEMORY, but only if the implementation is certain that the "
            "pool allocation failure was due to fragmentation."
        },
        {
            RTK_VK_RESULT_NAME(VK_ERROR_SURFACE_LOST_KHR),
            "A surface is no longer available."
        },
        {
            RTK_VK_RESULT_NAME(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR),
            "The requested window is already in use by Vulkan or another API in a manner which prevents it from being "
            "used again."
        },
        {
            RTK_VK_RESULT_NAME(VK_ERROR_OUT_OF_DATE_KHR),
            "A surface has changed in such a way that it is no longer compatible with the swapchain, and further "
            "presentation requests using the swapchain will fail. Applications must query the new surface properties "
            "and recreate their swapchain if they wish to continue presenting to the surface."
        },
        {
            RTK_VK_RESULT_NAME(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR),
            "The display used by a swapchain does not use the same presentable image layout, or is incompatible in a "
            "way that prevents sharing an image."
        },
        {
            RTK_VK_RESULT_NAME(VK_ERROR_INVALID_SHADER_NV),
            "One or more shaders failed to compile or link."
        },
        {
            RTK_VK_RESULT_NAME(VK_ERROR_OUT_OF_POOL_MEMORY),
            "A pool memory allocation has failed. This must only be returned if no attempt to allocate host or device "
            "memory was made to accommodate the new allocation. If the failure was definitely due to fragmentation of "
            "the pool, VK_ERROR_FRAGMENTED_POOL should be returned instead."
        },
        {
            RTK_VK_RESULT_NAME(VK_ERROR_INVALID_EXTERNAL_HANDLE),
            "An external handle is not a valid handle of the specified type."
        },
        // {
        //     RTK_VK_RESULT_NAME(VK_ERROR_FRAGMENTATION),
        //     "A descriptor pool creation has failed due to fragmentation."
        // },
        {
            RTK_VK_RESULT_NAME(VK_ERROR_INVALID_DEVICE_ADDRESS_EXT),
            "A buffer creation failed because the requested address is not available."
        },
        // {
        //     RTK_VK_RESULT_NAME(VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS),
        //     "A buffer creation or memory allocation failed because the requested address is not available."
        // },
        {
            RTK_VK_RESULT_NAME(VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT),
            "An operation on a swapchain created with VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT failed as it "
            "did not have exlusive full-screen access. This may occur due to implementation-dependent reasons, outside "
            "of the application’s control."
        },
        // {
        //     RTK_VK_RESULT_NAME(VK_ERROR_UNKNOWN),
        //     "An unknown error has occurred; either the application has provided invalid input, or an implementation "
        //     "failure has occurred."
        // },
    };

    const VkResultInfo* res_info = NULL;
    for (uint32 i = 0; i < CTK_ARRAY_SIZE(VK_RESULT_DEBUG_INFOS); ++i)
    {
        res_info = VK_RESULT_DEBUG_INFOS + i;
        if (res_info->result == result)
        {
            break;
        }
    }

    if (!res_info)
    {
        CTK_FATAL("failed to find debug info for VkResult %d", result);
    }

    if (res_info->result == 0)
    {
        PrintInfo("vulkan function returned %s: %s", res_info->name, res_info->message);
    }
    else if (res_info->result > 0)
    {
        PrintWarning("vulkan function returned %s: %s", res_info->name, res_info->message);
    }
    else
    {
        PrintError("vulkan function returned %s: %s", res_info->name, res_info->message);
    }
}

template<typename ...Args>
static void Validate(VkResult result, const char* fail_message, Args... args)
{
    if (result != VK_SUCCESS)
    {
        PrintResult(result);
        CTK_FATAL(fail_message, args...);
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL
DefaultDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity_flag_bit,
                     VkDebugUtilsMessageTypeFlagsEXT message_type_flags,
                     const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data)
{
    CTK_UNUSED(message_type_flags)
    CTK_UNUSED(user_data)

    uint32 msg_size = StringSize(callback_data->pMessage);
    uint32 index = 0;
    if (VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT & message_severity_flag_bit)
    {
        // Print formatted error message properties.
        Print(CTK_ERROR_TAG);

        index = PrintToChar(callback_data->pMessage, msg_size, ':', index, CTK_ANSI_COLOR_SKY);
        index = PrintToChar(callback_data->pMessage, msg_size, ']', index);
        index += 2;
        Print("]" CTK_ERROR_NL);

        // Print each object on separate line.
        while (StringsMatch(callback_data->pMessage + index, "Object", 6))
        {
            index = PrintToChar(callback_data->pMessage, msg_size, ':', index, CTK_ANSI_COLOR_SKY);
            index = PrintToChar(callback_data->pMessage, msg_size, ';', index);
            index += 2;
            Print(CTK_ERROR_NL);
        }
        index += 2;

        index = PrintToChar(callback_data->pMessage, msg_size, ' ', index, CTK_ANSI_COLOR_SKY);
        Print(':');
        index += 2;
        index = PrintToChar(callback_data->pMessage, msg_size, '|', index);
        index += 2;
        Print(CTK_ERROR_NL);

        // Print formatted error message text, word-wrapped to console width.
        static constexpr const char* MESSAGE_TAG = CTK_ANSI_HIGHLIGHT(SKY, "Message") ": ";
        static constexpr uint32 ERROR_TAG_SIZE   = 7;
        static constexpr uint32 MESSAGE_TAG_SIZE = 9;
        Print(MESSAGE_TAG);

        uint32 max_width = (uint32)GetConsoleScreenBufferWidth();
        uint32 first_line_start = ERROR_TAG_SIZE + MESSAGE_TAG_SIZE; // First line starts after message tag.
        CTK_ASSERT(max_width > first_line_start); // Must be room for atleast 1 character on first line.

        uint32 line_start = first_line_start;
        while (index < msg_size)
        {
            uint32 line_size = 0;
            uint32 print_size = 0;
            for (;;)
            {
                if (callback_data->pMessage[index + line_size] == ' ')
                {
                    ++line_size;
                    print_size = line_size;
                }

                ++line_size;
                if (index + line_size >= msg_size)
                {
                    // Print remaing characters in message.
                    print_size = msg_size - index;
                    break;
                }

                if (line_start + line_size >= max_width)
                {
                    if (print_size == 0)
                    {
                        print_size = max_width - line_start;
                    }
                    break;
                }
            }
            Print("%.*s", print_size, callback_data->pMessage + index);
            Print(CTK_ERROR_NL);
            index += print_size;
            line_start = ERROR_TAG_SIZE; // Subsequent lines start after error tag.
        }
        PrintLine();

        throw 0;
    }
    else if (VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT & message_severity_flag_bit)
    {
        PrintWarning(callback_data->pMessage);
    }
    else
    {
        // PrintInfo(callback_data->pMessage);
    }

    return VK_FALSE;
}

static void PrintMemoryRequirements(VkMemoryRequirements* mem_requirements, uint32 tabs = 0)
{
    PrintTabs(tabs);
    PrintLine("size:           %u", mem_requirements->size);

    PrintTabs(tabs);
    PrintLine("align:          %u", mem_requirements->alignment);

    PrintTabs(tabs);
    Print("memoryTypeBits: ");
    PrintBits((uint8*)&mem_requirements->memoryTypeBits, sizeof(mem_requirements->memoryTypeBits));
    PrintLine();
}

static void PrintMemoryProperties(uint32 mem_properties, uint32 tabs = 0)
{
    RTK_CHECK_PRINT_FLAG(mem_properties, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    RTK_CHECK_PRINT_FLAG(mem_properties, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    RTK_CHECK_PRINT_FLAG(mem_properties, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    RTK_CHECK_PRINT_FLAG(mem_properties, VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
    RTK_CHECK_PRINT_FLAG(mem_properties, VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT);
    RTK_CHECK_PRINT_FLAG(mem_properties, VK_MEMORY_PROPERTY_PROTECTED_BIT);
    RTK_CHECK_PRINT_FLAG(mem_properties, VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD);
    RTK_CHECK_PRINT_FLAG(mem_properties, VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD);
    RTK_CHECK_PRINT_FLAG(mem_properties, VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV);
}

static void PrintBufferUsage(uint32 usage, uint32 tabs = 0)
{
    RTK_CHECK_PRINT_FLAG(usage, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    RTK_CHECK_PRINT_FLAG(usage, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    RTK_CHECK_PRINT_FLAG(usage, VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT);
    RTK_CHECK_PRINT_FLAG(usage, VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT);
    RTK_CHECK_PRINT_FLAG(usage, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    RTK_CHECK_PRINT_FLAG(usage, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    RTK_CHECK_PRINT_FLAG(usage, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    RTK_CHECK_PRINT_FLAG(usage, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    RTK_CHECK_PRINT_FLAG(usage, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);
}

static constexpr const char* VkPresentModeName(VkPresentModeKHR present_mode)
{
    switch (present_mode)
    {
        RTK_ENUM_NAME_CASE(VK_PRESENT_MODE_IMMEDIATE_KHR)
        RTK_ENUM_NAME_CASE(VK_PRESENT_MODE_MAILBOX_KHR)
        RTK_ENUM_NAME_CASE(VK_PRESENT_MODE_FIFO_KHR)
        RTK_ENUM_NAME_CASE(VK_PRESENT_MODE_FIFO_RELAXED_KHR)
        RTK_ENUM_NAME_CASE(VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR)
        RTK_ENUM_NAME_CASE(VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR)
        default: CTK_FATAL("unknown VkPresentModeKHR value: %u", (uint32)present_mode);
    };
}

static constexpr const char* VkDescriptorTypeName(VkDescriptorType descriptor_type)
{
    switch (descriptor_type)
    {
        RTK_ENUM_NAME_CASE(VK_DESCRIPTOR_TYPE_SAMPLER)
        RTK_ENUM_NAME_CASE(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
        RTK_ENUM_NAME_CASE(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
        RTK_ENUM_NAME_CASE(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
        RTK_ENUM_NAME_CASE(VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER)
        RTK_ENUM_NAME_CASE(VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER)
        RTK_ENUM_NAME_CASE(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
        RTK_ENUM_NAME_CASE(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
        RTK_ENUM_NAME_CASE(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
        RTK_ENUM_NAME_CASE(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)
        RTK_ENUM_NAME_CASE(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT)
        default: CTK_FATAL("unknown VkDescriptorType value: %u", (uint32)descriptor_type);
    }
}

static constexpr const char* VkBufferUsageName(VkBufferUsageFlagBits buffer_usage)
{
    switch (buffer_usage)
    {
        RTK_ENUM_NAME_CASE(VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
        RTK_ENUM_NAME_CASE(VK_BUFFER_USAGE_TRANSFER_DST_BIT)
        RTK_ENUM_NAME_CASE(VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT)
        RTK_ENUM_NAME_CASE(VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT)
        RTK_ENUM_NAME_CASE(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
        RTK_ENUM_NAME_CASE(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
        RTK_ENUM_NAME_CASE(VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
        RTK_ENUM_NAME_CASE(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
        RTK_ENUM_NAME_CASE(VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT)
        default: CTK_FATAL("unknown VkBufferUsageFlagBits value: %u", (uint32)buffer_usage);
    }
}

static constexpr const char* VkVertexInputRateName(VkVertexInputRate input_rate)
{
    switch (input_rate)
    {
        RTK_ENUM_NAME_CASE(VK_VERTEX_INPUT_RATE_VERTEX)
        RTK_ENUM_NAME_CASE(VK_VERTEX_INPUT_RATE_INSTANCE)
        default: CTK_FATAL("unknown VkVertexInputRateName value: %u", (uint32)input_rate);
    }
}


static constexpr const char* VkResultName(VkResult result)
{
    switch (result)
    {
        RTK_ENUM_NAME_CASE(VK_SUCCESS)
        RTK_ENUM_NAME_CASE(VK_NOT_READY)
        RTK_ENUM_NAME_CASE(VK_TIMEOUT)
        RTK_ENUM_NAME_CASE(VK_EVENT_SET)
        RTK_ENUM_NAME_CASE(VK_EVENT_RESET)
        RTK_ENUM_NAME_CASE(VK_INCOMPLETE)
        RTK_ENUM_NAME_CASE(VK_ERROR_OUT_OF_HOST_MEMORY)
        RTK_ENUM_NAME_CASE(VK_ERROR_OUT_OF_DEVICE_MEMORY)
        RTK_ENUM_NAME_CASE(VK_ERROR_INITIALIZATION_FAILED)
        RTK_ENUM_NAME_CASE(VK_ERROR_DEVICE_LOST)
        RTK_ENUM_NAME_CASE(VK_ERROR_MEMORY_MAP_FAILED)
        RTK_ENUM_NAME_CASE(VK_ERROR_LAYER_NOT_PRESENT)
        RTK_ENUM_NAME_CASE(VK_ERROR_EXTENSION_NOT_PRESENT)
        RTK_ENUM_NAME_CASE(VK_ERROR_FEATURE_NOT_PRESENT)
        RTK_ENUM_NAME_CASE(VK_ERROR_INCOMPATIBLE_DRIVER)
        RTK_ENUM_NAME_CASE(VK_ERROR_TOO_MANY_OBJECTS)
        RTK_ENUM_NAME_CASE(VK_ERROR_FORMAT_NOT_SUPPORTED)
        RTK_ENUM_NAME_CASE(VK_ERROR_FRAGMENTED_POOL)
        RTK_ENUM_NAME_CASE(VK_ERROR_UNKNOWN)
        // Provided by VK_VERSION_1_1
        RTK_ENUM_NAME_CASE(VK_ERROR_OUT_OF_POOL_MEMORY)
        RTK_ENUM_NAME_CASE(VK_ERROR_INVALID_EXTERNAL_HANDLE)
        // Provided by VK_VERSION_1_2
        RTK_ENUM_NAME_CASE(VK_ERROR_FRAGMENTATION)
        RTK_ENUM_NAME_CASE(VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS)
        // Provided by VK_VERSION_1_3
        RTK_ENUM_NAME_CASE(VK_PIPELINE_COMPILE_REQUIRED)
        // Provided by VK_KHR_surface
        RTK_ENUM_NAME_CASE(VK_ERROR_SURFACE_LOST_KHR)
        RTK_ENUM_NAME_CASE(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR)
        // Provided by VK_KHR_swapchain
        RTK_ENUM_NAME_CASE(VK_SUBOPTIMAL_KHR)
        RTK_ENUM_NAME_CASE(VK_ERROR_OUT_OF_DATE_KHR)
        // Provided by VK_KHR_display_swapchain
        RTK_ENUM_NAME_CASE(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR)
        // Provided by VK_EXT_debug_report
        RTK_ENUM_NAME_CASE(VK_ERROR_VALIDATION_FAILED_EXT)
        // Provided by VK_NV_glsl_shader
        RTK_ENUM_NAME_CASE(VK_ERROR_INVALID_SHADER_NV)
        // Provided by VK_KHR_video_queue
        RTK_ENUM_NAME_CASE(VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR)
        RTK_ENUM_NAME_CASE(VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR)
        RTK_ENUM_NAME_CASE(VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR)
        RTK_ENUM_NAME_CASE(VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR)
        RTK_ENUM_NAME_CASE(VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR)
        RTK_ENUM_NAME_CASE(VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR)
        // Provided by VK_EXT_image_drm_format_modifier
        RTK_ENUM_NAME_CASE(VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT)
        // Provided by VK_KHR_global_priority
        RTK_ENUM_NAME_CASE(VK_ERROR_NOT_PERMITTED_KHR)
        // Provided by VK_EXT_full_screen_exclusive
        RTK_ENUM_NAME_CASE(VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT)
        // Provided by VK_KHR_deferred_host_operations
        RTK_ENUM_NAME_CASE(VK_THREAD_IDLE_KHR)
        RTK_ENUM_NAME_CASE(VK_THREAD_DONE_KHR)
        RTK_ENUM_NAME_CASE(VK_OPERATION_DEFERRED_KHR)
        RTK_ENUM_NAME_CASE(VK_OPERATION_NOT_DEFERRED_KHR)
#ifdef VK_ENABLE_BETA_EXTENSIONS
        // Provided by VK_KHR_video_encode_queue
        RTK_ENUM_NAME_CASE(VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR)
#endif
        // Provided by VK_EXT_image_compression_control
        RTK_ENUM_NAME_CASE(VK_ERROR_COMPRESSION_EXHAUSTED_EXT)
        // Provided by VK_EXT_shader_object
        RTK_ENUM_NAME_CASE(VK_ERROR_INCOMPATIBLE_SHADER_BINARY_EXT)

        ///
        /// Duplicates
        ///
        // // Provided by VK_KHR_maintenance1
        // RTK_ENUM_NAME_CASE(VK_ERROR_OUT_OF_POOL_MEMORY_KHR)
        // // Provided by VK_KHR_external_memory
        // RTK_ENUM_NAME_CASE(VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR)
        // // Provided by VK_EXT_descriptor_indexing
        // RTK_ENUM_NAME_CASE(VK_ERROR_FRAGMENTATION_EXT)
        // // Provided by VK_EXT_global_priority
        // RTK_ENUM_NAME_CASE(VK_ERROR_NOT_PERMITTED_EXT)
        // // Provided by VK_EXT_buffer_device_address
        // RTK_ENUM_NAME_CASE(VK_ERROR_INVALID_DEVICE_ADDRESS_EXT)
        // // Provided by VK_KHR_buffer_device_address
        // RTK_ENUM_NAME_CASE(VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR)
        // // Provided by VK_EXT_pipeline_creation_cache_control
        // RTK_ENUM_NAME_CASE(VK_PIPELINE_COMPILE_REQUIRED_EXT)
        // RTK_ENUM_NAME_CASE(VK_ERROR_PIPELINE_COMPILE_REQUIRED_EXT)

        default: CTK_FATAL("unknown VkResult value: %u", (uint32)result);
    }
}
