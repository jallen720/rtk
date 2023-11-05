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
        Print("%s%.*s" CTK_ANSI_RESET, color, index - start, &msg[start]);
    }
    else
    {
        Print("%.*s" CTK_ANSI_RESET, index - start, &msg[start]);
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
        while (StringsMatch(&callback_data->pMessage[index], "Object", 6))
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
            Print("%.*s", print_size, &callback_data->pMessage[index]);
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

/// Misc
////////////////////////////////////////////////////////////
static void PrintMemoryRequirements(VkMemoryRequirements* mem_requirements, uint32 tabs = 0)
{
    PrintTabs(tabs);
    PrintLine    ("size:           %u", mem_requirements->size);
    PrintTabs(tabs);
    PrintLine    ("alignment:      %u", mem_requirements->alignment);
    PrintTabs(tabs);
    PrintBitsLine(mem_requirements->memoryTypeBits,
                  "memoryTypeBits: ");
}

/// Flag Printing
////////////////////////////////////////////////////////////
static void PrintMemoryPropertyFlags(VkMemoryPropertyFlags mem_property_flags, uint32 tabs = 0)
{
    RTK_CHECK_PRINT_FLAG(mem_property_flags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    RTK_CHECK_PRINT_FLAG(mem_property_flags, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    RTK_CHECK_PRINT_FLAG(mem_property_flags, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    RTK_CHECK_PRINT_FLAG(mem_property_flags, VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
    RTK_CHECK_PRINT_FLAG(mem_property_flags, VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT);
    RTK_CHECK_PRINT_FLAG(mem_property_flags, VK_MEMORY_PROPERTY_PROTECTED_BIT);
    RTK_CHECK_PRINT_FLAG(mem_property_flags, VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD);
    RTK_CHECK_PRINT_FLAG(mem_property_flags, VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD);
    RTK_CHECK_PRINT_FLAG(mem_property_flags, VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV);
}

static void PrintBufferUsageFlags(VkBufferUsageFlags buffer_usage_flags, uint32 tabs = 0)
{
    RTK_CHECK_PRINT_FLAG(buffer_usage_flags, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    RTK_CHECK_PRINT_FLAG(buffer_usage_flags, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    RTK_CHECK_PRINT_FLAG(buffer_usage_flags, VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT);
    RTK_CHECK_PRINT_FLAG(buffer_usage_flags, VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT);
    RTK_CHECK_PRINT_FLAG(buffer_usage_flags, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    RTK_CHECK_PRINT_FLAG(buffer_usage_flags, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    RTK_CHECK_PRINT_FLAG(buffer_usage_flags, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    RTK_CHECK_PRINT_FLAG(buffer_usage_flags, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    RTK_CHECK_PRINT_FLAG(buffer_usage_flags, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);
}

static void PrintImageUsageFlags(VkImageUsageFlags image_usage_flags, uint32 tabs = 0)
{
    RTK_CHECK_PRINT_FLAG(image_usage_flags, VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
    RTK_CHECK_PRINT_FLAG(image_usage_flags, VK_IMAGE_USAGE_TRANSFER_DST_BIT)
    RTK_CHECK_PRINT_FLAG(image_usage_flags, VK_IMAGE_USAGE_SAMPLED_BIT)
    RTK_CHECK_PRINT_FLAG(image_usage_flags, VK_IMAGE_USAGE_STORAGE_BIT)
    RTK_CHECK_PRINT_FLAG(image_usage_flags, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
    RTK_CHECK_PRINT_FLAG(image_usage_flags, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
    RTK_CHECK_PRINT_FLAG(image_usage_flags, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT)
    RTK_CHECK_PRINT_FLAG(image_usage_flags, VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)
    // Provided by VK_KHR_video_decode_queue
    RTK_CHECK_PRINT_FLAG(image_usage_flags, VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR)
    RTK_CHECK_PRINT_FLAG(image_usage_flags, VK_IMAGE_USAGE_VIDEO_DECODE_SRC_BIT_KHR)
    RTK_CHECK_PRINT_FLAG(image_usage_flags, VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR)
    // Provided by VK_EXT_fragment_density_map
    RTK_CHECK_PRINT_FLAG(image_usage_flags, VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT)
    // Provided by VK_KHR_fragment_shading_rate
    RTK_CHECK_PRINT_FLAG(image_usage_flags, VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR)
    // Provided by VK_EXT_host_image_copy
    RTK_CHECK_PRINT_FLAG(image_usage_flags, VK_IMAGE_USAGE_HOST_TRANSFER_BIT_EXT)
    // Provided by VK_KHR_video_encode_queue
#ifdef VK_ENABLE_BETA_EXTENSIONS
    RTK_CHECK_PRINT_FLAG(image_usage_flags, VK_IMAGE_USAGE_VIDEO_ENCODE_DST_BIT_KHR)
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
    RTK_CHECK_PRINT_FLAG(image_usage_flags, VK_IMAGE_USAGE_VIDEO_ENCODE_SRC_BIT_KHR)
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
    RTK_CHECK_PRINT_FLAG(image_usage_flags, VK_IMAGE_USAGE_VIDEO_ENCODE_DPB_BIT_KHR)
#endif
    // Provided by VK_EXT_attachment_feedback_loop_layout
    RTK_CHECK_PRINT_FLAG(image_usage_flags, VK_IMAGE_USAGE_ATTACHMENT_FEEDBACK_LOOP_BIT_EXT)
    // Provided by VK_HUAWEI_invocation_mask
    RTK_CHECK_PRINT_FLAG(image_usage_flags, VK_IMAGE_USAGE_INVOCATION_MASK_BIT_HUAWEI)
    // Provided by VK_QCOM_image_processing
    RTK_CHECK_PRINT_FLAG(image_usage_flags, VK_IMAGE_USAGE_SAMPLE_WEIGHT_BIT_QCOM)
    RTK_CHECK_PRINT_FLAG(image_usage_flags, VK_IMAGE_USAGE_SAMPLE_BLOCK_MATCH_BIT_QCOM)
}

static void PrintImageCreateFlags(VkImageCreateFlags image_create_flags, uint32 tabs = 0)
{
    RTK_CHECK_PRINT_FLAG(image_create_flags, VK_IMAGE_CREATE_SPARSE_BINDING_BIT)
    RTK_CHECK_PRINT_FLAG(image_create_flags, VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT)
    RTK_CHECK_PRINT_FLAG(image_create_flags, VK_IMAGE_CREATE_SPARSE_ALIASED_BIT)
    RTK_CHECK_PRINT_FLAG(image_create_flags, VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT)
    RTK_CHECK_PRINT_FLAG(image_create_flags, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)
    // Provided by VK_VERSION_1_1
    RTK_CHECK_PRINT_FLAG(image_create_flags, VK_IMAGE_CREATE_ALIAS_BIT)
    RTK_CHECK_PRINT_FLAG(image_create_flags, VK_IMAGE_CREATE_SPLIT_INSTANCE_BIND_REGIONS_BIT)
    RTK_CHECK_PRINT_FLAG(image_create_flags, VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT)
    RTK_CHECK_PRINT_FLAG(image_create_flags, VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT)
    RTK_CHECK_PRINT_FLAG(image_create_flags, VK_IMAGE_CREATE_EXTENDED_USAGE_BIT)
    RTK_CHECK_PRINT_FLAG(image_create_flags, VK_IMAGE_CREATE_PROTECTED_BIT)
    RTK_CHECK_PRINT_FLAG(image_create_flags, VK_IMAGE_CREATE_DISJOINT_BIT)
    // Provided by VK_NV_corner_sampled_image
    RTK_CHECK_PRINT_FLAG(image_create_flags, VK_IMAGE_CREATE_CORNER_SAMPLED_BIT_NV)
    // Provided by VK_EXT_sample_locations
    RTK_CHECK_PRINT_FLAG(image_create_flags, VK_IMAGE_CREATE_SAMPLE_LOCATIONS_COMPATIBLE_DEPTH_BIT_EXT)
    // Provided by VK_EXT_fragment_density_map
    RTK_CHECK_PRINT_FLAG(image_create_flags, VK_IMAGE_CREATE_SUBSAMPLED_BIT_EXT)
    // Provided by VK_EXT_descriptor_buffer
    RTK_CHECK_PRINT_FLAG(image_create_flags, VK_IMAGE_CREATE_DESCRIPTOR_BUFFER_CAPTURE_REPLAY_BIT_EXT)
    // Provided by VK_EXT_multisampled_render_to_single_sampled
    RTK_CHECK_PRINT_FLAG(image_create_flags, VK_IMAGE_CREATE_MULTISAMPLED_RENDER_TO_SINGLE_SAMPLED_BIT_EXT)
    // Provided by VK_EXT_image_2d_view_of_3d
    RTK_CHECK_PRINT_FLAG(image_create_flags, VK_IMAGE_CREATE_2D_VIEW_COMPATIBLE_BIT_EXT)
    // Provided by VK_QCOM_fragment_density_map_offset
    RTK_CHECK_PRINT_FLAG(image_create_flags, VK_IMAGE_CREATE_FRAGMENT_DENSITY_MAP_OFFSET_BIT_QCOM)
}

static void PrintImageAspectFlags(VkImageAspectFlags image_aspect_flags, uint32 tabs = 0)
{
    RTK_CHECK_PRINT_FLAG(image_aspect_flags, VK_IMAGE_ASPECT_COLOR_BIT)
    RTK_CHECK_PRINT_FLAG(image_aspect_flags, VK_IMAGE_ASPECT_DEPTH_BIT)
    RTK_CHECK_PRINT_FLAG(image_aspect_flags, VK_IMAGE_ASPECT_STENCIL_BIT)
    RTK_CHECK_PRINT_FLAG(image_aspect_flags, VK_IMAGE_ASPECT_METADATA_BIT)
    // Provided by VK_VERSION_1_1
    RTK_CHECK_PRINT_FLAG(image_aspect_flags, VK_IMAGE_ASPECT_PLANE_0_BIT)
    RTK_CHECK_PRINT_FLAG(image_aspect_flags, VK_IMAGE_ASPECT_PLANE_1_BIT)
    RTK_CHECK_PRINT_FLAG(image_aspect_flags, VK_IMAGE_ASPECT_PLANE_2_BIT)
    // Provided by VK_VERSION_1_3
    RTK_CHECK_PRINT_FLAG(image_aspect_flags, VK_IMAGE_ASPECT_NONE)
    // Provided by VK_EXT_image_drm_format_modifier
    RTK_CHECK_PRINT_FLAG(image_aspect_flags, VK_IMAGE_ASPECT_MEMORY_PLANE_0_BIT_EXT)
    RTK_CHECK_PRINT_FLAG(image_aspect_flags, VK_IMAGE_ASPECT_MEMORY_PLANE_1_BIT_EXT)
    RTK_CHECK_PRINT_FLAG(image_aspect_flags, VK_IMAGE_ASPECT_MEMORY_PLANE_2_BIT_EXT)
    RTK_CHECK_PRINT_FLAG(image_aspect_flags, VK_IMAGE_ASPECT_MEMORY_PLANE_3_BIT_EXT)
}

static void PrintImageViewCreateFlags(VkImageViewCreateFlags image_view_create_flags, uint32 tabs = 0)
{
    // Provided by VK_EXT_fragment_density_map
    RTK_CHECK_PRINT_FLAG(image_view_create_flags, VK_IMAGE_VIEW_CREATE_FRAGMENT_DENSITY_MAP_DYNAMIC_BIT_EXT)
    // Provided by VK_EXT_descriptor_buffer
    RTK_CHECK_PRINT_FLAG(image_view_create_flags, VK_IMAGE_VIEW_CREATE_DESCRIPTOR_BUFFER_CAPTURE_REPLAY_BIT_EXT)
    // Provided by VK_EXT_fragment_density_map2
    RTK_CHECK_PRINT_FLAG(image_view_create_flags, VK_IMAGE_VIEW_CREATE_FRAGMENT_DENSITY_MAP_DEFERRED_BIT_EXT)
}

/// Enum Strings
////////////////////////////////////////////////////////////
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

static constexpr const char* VkImageCreateName(VkImageCreateFlagBits image_create)
{
    switch (image_create)
    {
        RTK_ENUM_NAME_CASE(VK_IMAGE_CREATE_SPARSE_BINDING_BIT)
        RTK_ENUM_NAME_CASE(VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT)
        RTK_ENUM_NAME_CASE(VK_IMAGE_CREATE_SPARSE_ALIASED_BIT)
        RTK_ENUM_NAME_CASE(VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT)
        RTK_ENUM_NAME_CASE(VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)
        // Provided by VK_VERSION_1_1
        RTK_ENUM_NAME_CASE(VK_IMAGE_CREATE_ALIAS_BIT)
        RTK_ENUM_NAME_CASE(VK_IMAGE_CREATE_SPLIT_INSTANCE_BIND_REGIONS_BIT)
        RTK_ENUM_NAME_CASE(VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT)
        RTK_ENUM_NAME_CASE(VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT)
        RTK_ENUM_NAME_CASE(VK_IMAGE_CREATE_EXTENDED_USAGE_BIT)
        RTK_ENUM_NAME_CASE(VK_IMAGE_CREATE_PROTECTED_BIT)
        RTK_ENUM_NAME_CASE(VK_IMAGE_CREATE_DISJOINT_BIT)
        // Provided by VK_NV_corner_sampled_image
        RTK_ENUM_NAME_CASE(VK_IMAGE_CREATE_CORNER_SAMPLED_BIT_NV)
        // Provided by VK_EXT_sample_locations
        RTK_ENUM_NAME_CASE(VK_IMAGE_CREATE_SAMPLE_LOCATIONS_COMPATIBLE_DEPTH_BIT_EXT)
        // Provided by VK_EXT_fragment_density_map
        RTK_ENUM_NAME_CASE(VK_IMAGE_CREATE_SUBSAMPLED_BIT_EXT)
        // Provided by VK_EXT_descriptor_buffer
        RTK_ENUM_NAME_CASE(VK_IMAGE_CREATE_DESCRIPTOR_BUFFER_CAPTURE_REPLAY_BIT_EXT)
        // Provided by VK_EXT_multisampled_render_to_single_sampled
        RTK_ENUM_NAME_CASE(VK_IMAGE_CREATE_MULTISAMPLED_RENDER_TO_SINGLE_SAMPLED_BIT_EXT)
        // Provided by VK_EXT_image_2d_view_of_3d
        RTK_ENUM_NAME_CASE(VK_IMAGE_CREATE_2D_VIEW_COMPATIBLE_BIT_EXT)
        // Provided by VK_QCOM_fragment_density_map_offset
        RTK_ENUM_NAME_CASE(VK_IMAGE_CREATE_FRAGMENT_DENSITY_MAP_OFFSET_BIT_QCOM)
        default: CTK_FATAL("unknown VkImageCreateFlagBits value: %u", (uint32)image_create);
    }
}

static constexpr const char* VkImageTypeName(VkImageType image_type)
{
    switch (image_type)
    {
        RTK_ENUM_NAME_CASE(VK_IMAGE_TYPE_1D)
        RTK_ENUM_NAME_CASE(VK_IMAGE_TYPE_2D)
        RTK_ENUM_NAME_CASE(VK_IMAGE_TYPE_3D)
        default: CTK_FATAL("unknown VkImageType value: %u", (uint32)image_type);
    }
}

static constexpr const char* VkFormatName(VkFormat format)
{
    switch (format)
    {
        RTK_ENUM_NAME_CASE(VK_FORMAT_UNDEFINED)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R4G4_UNORM_PACK8)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R4G4B4A4_UNORM_PACK16)
        RTK_ENUM_NAME_CASE(VK_FORMAT_B4G4R4A4_UNORM_PACK16)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R5G6B5_UNORM_PACK16)
        RTK_ENUM_NAME_CASE(VK_FORMAT_B5G6R5_UNORM_PACK16)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R5G5B5A1_UNORM_PACK16)
        RTK_ENUM_NAME_CASE(VK_FORMAT_B5G5R5A1_UNORM_PACK16)
        RTK_ENUM_NAME_CASE(VK_FORMAT_A1R5G5B5_UNORM_PACK16)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R8_UNORM)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R8_SNORM)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R8_USCALED)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R8_SSCALED)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R8_UINT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R8_SINT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R8_SRGB)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R8G8_UNORM)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R8G8_SNORM)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R8G8_USCALED)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R8G8_SSCALED)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R8G8_UINT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R8G8_SINT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R8G8_SRGB)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R8G8B8_UNORM)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R8G8B8_SNORM)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R8G8B8_USCALED)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R8G8B8_SSCALED)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R8G8B8_UINT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R8G8B8_SINT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R8G8B8_SRGB)
        RTK_ENUM_NAME_CASE(VK_FORMAT_B8G8R8_UNORM)
        RTK_ENUM_NAME_CASE(VK_FORMAT_B8G8R8_SNORM)
        RTK_ENUM_NAME_CASE(VK_FORMAT_B8G8R8_USCALED)
        RTK_ENUM_NAME_CASE(VK_FORMAT_B8G8R8_SSCALED)
        RTK_ENUM_NAME_CASE(VK_FORMAT_B8G8R8_UINT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_B8G8R8_SINT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_B8G8R8_SRGB)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R8G8B8A8_UNORM)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R8G8B8A8_SNORM)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R8G8B8A8_USCALED)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R8G8B8A8_SSCALED)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R8G8B8A8_UINT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R8G8B8A8_SINT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R8G8B8A8_SRGB)
        RTK_ENUM_NAME_CASE(VK_FORMAT_B8G8R8A8_UNORM)
        RTK_ENUM_NAME_CASE(VK_FORMAT_B8G8R8A8_SNORM)
        RTK_ENUM_NAME_CASE(VK_FORMAT_B8G8R8A8_USCALED)
        RTK_ENUM_NAME_CASE(VK_FORMAT_B8G8R8A8_SSCALED)
        RTK_ENUM_NAME_CASE(VK_FORMAT_B8G8R8A8_UINT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_B8G8R8A8_SINT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_B8G8R8A8_SRGB)
        RTK_ENUM_NAME_CASE(VK_FORMAT_A8B8G8R8_UNORM_PACK32)
        RTK_ENUM_NAME_CASE(VK_FORMAT_A8B8G8R8_SNORM_PACK32)
        RTK_ENUM_NAME_CASE(VK_FORMAT_A8B8G8R8_USCALED_PACK32)
        RTK_ENUM_NAME_CASE(VK_FORMAT_A8B8G8R8_SSCALED_PACK32)
        RTK_ENUM_NAME_CASE(VK_FORMAT_A8B8G8R8_UINT_PACK32)
        RTK_ENUM_NAME_CASE(VK_FORMAT_A8B8G8R8_SINT_PACK32)
        RTK_ENUM_NAME_CASE(VK_FORMAT_A8B8G8R8_SRGB_PACK32)
        RTK_ENUM_NAME_CASE(VK_FORMAT_A2R10G10B10_UNORM_PACK32)
        RTK_ENUM_NAME_CASE(VK_FORMAT_A2R10G10B10_SNORM_PACK32)
        RTK_ENUM_NAME_CASE(VK_FORMAT_A2R10G10B10_USCALED_PACK32)
        RTK_ENUM_NAME_CASE(VK_FORMAT_A2R10G10B10_SSCALED_PACK32)
        RTK_ENUM_NAME_CASE(VK_FORMAT_A2R10G10B10_UINT_PACK32)
        RTK_ENUM_NAME_CASE(VK_FORMAT_A2R10G10B10_SINT_PACK32)
        RTK_ENUM_NAME_CASE(VK_FORMAT_A2B10G10R10_UNORM_PACK32)
        RTK_ENUM_NAME_CASE(VK_FORMAT_A2B10G10R10_SNORM_PACK32)
        RTK_ENUM_NAME_CASE(VK_FORMAT_A2B10G10R10_USCALED_PACK32)
        RTK_ENUM_NAME_CASE(VK_FORMAT_A2B10G10R10_SSCALED_PACK32)
        RTK_ENUM_NAME_CASE(VK_FORMAT_A2B10G10R10_UINT_PACK32)
        RTK_ENUM_NAME_CASE(VK_FORMAT_A2B10G10R10_SINT_PACK32)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R16_UNORM)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R16_SNORM)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R16_USCALED)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R16_SSCALED)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R16_UINT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R16_SINT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R16_SFLOAT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R16G16_UNORM)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R16G16_SNORM)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R16G16_USCALED)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R16G16_SSCALED)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R16G16_UINT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R16G16_SINT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R16G16_SFLOAT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R16G16B16_UNORM)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R16G16B16_SNORM)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R16G16B16_USCALED)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R16G16B16_SSCALED)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R16G16B16_UINT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R16G16B16_SINT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R16G16B16_SFLOAT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R16G16B16A16_UNORM)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R16G16B16A16_SNORM)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R16G16B16A16_USCALED)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R16G16B16A16_SSCALED)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R16G16B16A16_UINT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R16G16B16A16_SINT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R16G16B16A16_SFLOAT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R32_UINT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R32_SINT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R32_SFLOAT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R32G32_UINT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R32G32_SINT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R32G32_SFLOAT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R32G32B32_UINT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R32G32B32_SINT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R32G32B32_SFLOAT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R32G32B32A32_UINT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R32G32B32A32_SINT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R32G32B32A32_SFLOAT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R64_UINT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R64_SINT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R64_SFLOAT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R64G64_UINT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R64G64_SINT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R64G64_SFLOAT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R64G64B64_UINT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R64G64B64_SINT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R64G64B64_SFLOAT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R64G64B64A64_UINT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R64G64B64A64_SINT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R64G64B64A64_SFLOAT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_B10G11R11_UFLOAT_PACK32)
        RTK_ENUM_NAME_CASE(VK_FORMAT_E5B9G9R9_UFLOAT_PACK32)
        RTK_ENUM_NAME_CASE(VK_FORMAT_D16_UNORM)
        RTK_ENUM_NAME_CASE(VK_FORMAT_X8_D24_UNORM_PACK32)
        RTK_ENUM_NAME_CASE(VK_FORMAT_D32_SFLOAT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_S8_UINT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_D16_UNORM_S8_UINT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_D24_UNORM_S8_UINT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_D32_SFLOAT_S8_UINT)
        RTK_ENUM_NAME_CASE(VK_FORMAT_BC1_RGB_UNORM_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_BC1_RGB_SRGB_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_BC1_RGBA_UNORM_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_BC1_RGBA_SRGB_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_BC2_UNORM_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_BC2_SRGB_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_BC3_UNORM_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_BC3_SRGB_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_BC4_UNORM_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_BC4_SNORM_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_BC5_UNORM_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_BC5_SNORM_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_BC6H_UFLOAT_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_BC6H_SFLOAT_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_BC7_UNORM_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_BC7_SRGB_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_EAC_R11_UNORM_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_EAC_R11_SNORM_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_EAC_R11G11_UNORM_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_EAC_R11G11_SNORM_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ASTC_4x4_UNORM_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ASTC_4x4_SRGB_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ASTC_5x4_UNORM_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ASTC_5x4_SRGB_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ASTC_5x5_UNORM_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ASTC_5x5_SRGB_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ASTC_6x5_UNORM_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ASTC_6x5_SRGB_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ASTC_6x6_UNORM_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ASTC_6x6_SRGB_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ASTC_8x5_UNORM_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ASTC_8x5_SRGB_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ASTC_8x6_UNORM_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ASTC_8x6_SRGB_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ASTC_8x8_UNORM_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ASTC_8x8_SRGB_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ASTC_10x5_UNORM_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ASTC_10x5_SRGB_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ASTC_10x6_UNORM_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ASTC_10x6_SRGB_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ASTC_10x8_UNORM_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ASTC_10x8_SRGB_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ASTC_10x10_UNORM_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ASTC_10x10_SRGB_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ASTC_12x10_UNORM_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ASTC_12x10_SRGB_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ASTC_12x12_UNORM_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ASTC_12x12_SRGB_BLOCK)
        // Provided by VK_VERSION_1_1
        RTK_ENUM_NAME_CASE(VK_FORMAT_G8B8G8R8_422_UNORM)
        RTK_ENUM_NAME_CASE(VK_FORMAT_B8G8R8G8_422_UNORM)
        RTK_ENUM_NAME_CASE(VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM)
        RTK_ENUM_NAME_CASE(VK_FORMAT_G8_B8R8_2PLANE_420_UNORM)
        RTK_ENUM_NAME_CASE(VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM)
        RTK_ENUM_NAME_CASE(VK_FORMAT_G8_B8R8_2PLANE_422_UNORM)
        RTK_ENUM_NAME_CASE(VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R10X6_UNORM_PACK16)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R10X6G10X6_UNORM_2PACK16)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16)
        RTK_ENUM_NAME_CASE(VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16)
        RTK_ENUM_NAME_CASE(VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16)
        RTK_ENUM_NAME_CASE(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16)
        RTK_ENUM_NAME_CASE(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16)
        RTK_ENUM_NAME_CASE(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16)
        RTK_ENUM_NAME_CASE(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16)
        RTK_ENUM_NAME_CASE(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R12X4_UNORM_PACK16)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R12X4G12X4_UNORM_2PACK16)
        RTK_ENUM_NAME_CASE(VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16)
        RTK_ENUM_NAME_CASE(VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16)
        RTK_ENUM_NAME_CASE(VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16)
        RTK_ENUM_NAME_CASE(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16)
        RTK_ENUM_NAME_CASE(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16)
        RTK_ENUM_NAME_CASE(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16)
        RTK_ENUM_NAME_CASE(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16)
        RTK_ENUM_NAME_CASE(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16)
        RTK_ENUM_NAME_CASE(VK_FORMAT_G16B16G16R16_422_UNORM)
        RTK_ENUM_NAME_CASE(VK_FORMAT_B16G16R16G16_422_UNORM)
        RTK_ENUM_NAME_CASE(VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM)
        RTK_ENUM_NAME_CASE(VK_FORMAT_G16_B16R16_2PLANE_420_UNORM)
        RTK_ENUM_NAME_CASE(VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM)
        RTK_ENUM_NAME_CASE(VK_FORMAT_G16_B16R16_2PLANE_422_UNORM)
        RTK_ENUM_NAME_CASE(VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM)
        // Provided by VK_VERSION_1_3
        RTK_ENUM_NAME_CASE(VK_FORMAT_G8_B8R8_2PLANE_444_UNORM)
        RTK_ENUM_NAME_CASE(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16)
        RTK_ENUM_NAME_CASE(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16)
        RTK_ENUM_NAME_CASE(VK_FORMAT_G16_B16R16_2PLANE_444_UNORM)
        RTK_ENUM_NAME_CASE(VK_FORMAT_A4R4G4B4_UNORM_PACK16)
        RTK_ENUM_NAME_CASE(VK_FORMAT_A4B4G4R4_UNORM_PACK16)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK)
        RTK_ENUM_NAME_CASE(VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK)
        // Provided by VK_IMG_format_pvrtc
        RTK_ENUM_NAME_CASE(VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG)
        RTK_ENUM_NAME_CASE(VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG)
        RTK_ENUM_NAME_CASE(VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG)
        RTK_ENUM_NAME_CASE(VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG)
        RTK_ENUM_NAME_CASE(VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG)
        RTK_ENUM_NAME_CASE(VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG)
        RTK_ENUM_NAME_CASE(VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG)
        RTK_ENUM_NAME_CASE(VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG)
        // Provided by VK_NV_optical_flow
        RTK_ENUM_NAME_CASE(VK_FORMAT_R16G16_S10_5_NV)
        // Provided by VK_KHR_maintenance5
        RTK_ENUM_NAME_CASE(VK_FORMAT_A1B5G5R5_UNORM_PACK16_KHR)
        RTK_ENUM_NAME_CASE(VK_FORMAT_A8_UNORM_KHR)
        default: CTK_FATAL("unknown VkFormat value: %u", (uint32)format);
    }
}

static constexpr const char* VkSampleCountName(VkSampleCountFlagBits sample_count)
{
    switch (sample_count)
    {
        RTK_ENUM_NAME_CASE(VK_SAMPLE_COUNT_1_BIT)
        RTK_ENUM_NAME_CASE(VK_SAMPLE_COUNT_2_BIT)
        RTK_ENUM_NAME_CASE(VK_SAMPLE_COUNT_4_BIT)
        RTK_ENUM_NAME_CASE(VK_SAMPLE_COUNT_8_BIT)
        RTK_ENUM_NAME_CASE(VK_SAMPLE_COUNT_16_BIT)
        RTK_ENUM_NAME_CASE(VK_SAMPLE_COUNT_32_BIT)
        RTK_ENUM_NAME_CASE(VK_SAMPLE_COUNT_64_BIT)
        default: CTK_FATAL("unknown VkSampleCountFlagBits value: %u", (uint32)sample_count);
    }
}

static constexpr const char* VkImageTilingName(VkImageTiling image_tiling)
{
    switch (image_tiling)
    {
        RTK_ENUM_NAME_CASE(VK_IMAGE_TILING_OPTIMAL)
        RTK_ENUM_NAME_CASE(VK_IMAGE_TILING_LINEAR)
        // Provided by VK_EXT_image_drm_format_modifier
        RTK_ENUM_NAME_CASE(VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT)
        default: CTK_FATAL("unknown VkImageTiling value: %u", (uint32)image_tiling);
    }
}

static constexpr const char* VkImageViewTypeName(VkImageViewType image_view_type)
{
    switch (image_view_type)
    {
        RTK_ENUM_NAME_CASE(VK_IMAGE_VIEW_TYPE_1D)
        RTK_ENUM_NAME_CASE(VK_IMAGE_VIEW_TYPE_2D)
        RTK_ENUM_NAME_CASE(VK_IMAGE_VIEW_TYPE_3D)
        RTK_ENUM_NAME_CASE(VK_IMAGE_VIEW_TYPE_CUBE)
        RTK_ENUM_NAME_CASE(VK_IMAGE_VIEW_TYPE_1D_ARRAY)
        RTK_ENUM_NAME_CASE(VK_IMAGE_VIEW_TYPE_2D_ARRAY)
        RTK_ENUM_NAME_CASE(VK_IMAGE_VIEW_TYPE_CUBE_ARRAY)
        default: CTK_FATAL("unknown VkImageViewType value: %u", (uint32)image_view_type);
    }
}

static constexpr const char* VkComponentSwizzleName(VkComponentSwizzle component_swizzle)
{
    switch (component_swizzle)
    {
        RTK_ENUM_NAME_CASE(VK_COMPONENT_SWIZZLE_IDENTITY)
        RTK_ENUM_NAME_CASE(VK_COMPONENT_SWIZZLE_ZERO)
        RTK_ENUM_NAME_CASE(VK_COMPONENT_SWIZZLE_ONE)
        RTK_ENUM_NAME_CASE(VK_COMPONENT_SWIZZLE_R)
        RTK_ENUM_NAME_CASE(VK_COMPONENT_SWIZZLE_G)
        RTK_ENUM_NAME_CASE(VK_COMPONENT_SWIZZLE_B)
        RTK_ENUM_NAME_CASE(VK_COMPONENT_SWIZZLE_A)
        default: CTK_FATAL("unknown VkComponentSwizzle value: %u", (uint32)component_swizzle);
    }
}
