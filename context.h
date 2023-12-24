/// Macros
////////////////////////////////////////////////////////////
#define RTK_LOAD_INSTANCE_EXTENSION_FUNCTION(INSTANCE, FUNC_NAME) \
    auto FUNC_NAME = (PFN_ ## FUNC_NAME)vkGetInstanceProcAddr(INSTANCE, #FUNC_NAME); \
    if (FUNC_NAME == NULL) \
    { \
        CTK_FATAL("failed to load instance extension function \"%s\"", #FUNC_NAME); \
    }

/// Data
////////////////////////////////////////////////////////////
static constexpr uint32 UNSET_INDEX         = UINT32_MAX;
static constexpr uint32 MAX_DEVICE_FEATURES = sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32);
static constexpr uint32 MAX_FRAME_COUNT     = 4;

struct InstanceInfo
{
    const char*                         application_name;
    uint32                              api_version;
    Array<const char*>                  extensions;
    DebugCallback                       debug_callback;
    VkDebugUtilsMessageSeverityFlagsEXT debug_message_severity;
    VkDebugUtilsMessageTypeFlagsEXT     debug_message_type;
};

struct ContextInfo
{
    InstanceInfo   instance_info;
    uint32         render_thread_count;
    DeviceFeatures enabled_features;
};

struct QueueFamilies
{
    uint32 graphics;
    uint32 present;
};

struct ResourceSharing
{
    VkSharingMode mode;
    uint32        queue_family_index_count;
    const uint32* queue_family_indexes;
};

struct PhysicalDevice
{
    VkPhysicalDevice                 hnd;
    VkFormat                         depth_image_format;
    QueueFamilies                    queue_families;
    ResourceSharing                  resource_sharing;
    VkPhysicalDeviceProperties       properties;
    VkPhysicalDeviceMemoryProperties mem_properties;
    DeviceFeatures                   features;
};

struct Swapchain
{
    // Surface Config
    VkSurfaceFormatKHR            surface_format;
    VkPresentModeKHR              surface_present_mode;
    uint32                        surface_min_image_count;
    VkExtent2D                    surface_extent;
    VkSurfaceTransformFlagBitsKHR surface_transform;

    // Image Sharing Mode Config
    VkSharingMode image_sharing_mode;
    uint32        queue_family_index_count;
    uint32*       queue_family_indexes;

    // State
    VkSwapchainKHR     hnd;
    Array<VkImageView> image_views;
};

struct Frame
{
    // Sync State
    VkSemaphore image_acquired;
    VkSemaphore render_finished;
    VkFence     command_buffers_complete;

    // Render State
    VkCommandBuffer        primary_render_command_buffer;
    Array<VkCommandBuffer> render_command_buffers;
    uint32                 swapchain_image_index;
};

struct Context
{
    uint32 render_thread_count;

    // Instance State
    VkInstance               instance;
    VkDebugUtilsMessengerEXT debug_messenger;

    // Device State
    VkSurfaceKHR          surface;
    Array<PhysicalDevice> physical_devices;
    PhysicalDevice*       physical_device;
    VkDevice              device;
    VkQueue               graphics_queue;
    VkQueue               present_queue;
    VkCommandPool         main_command_pool;
    VkCommandBuffer       temp_command_buffer;

    // Render State
    Swapchain            swapchain;
    Array<VkCommandPool> render_command_pools;
    RingBuffer<Frame>    frames;
};

/// Instance
////////////////////////////////////////////////////////////
static Context g_context;

/// Forward Declarations
////////////////////////////////////////////////////////////
static void GetSurfaceCapabilities(VkSurfaceCapabilitiesKHR* capabilities);

/// Debugging
////////////////////////////////////////////////////////////
static void LogMemoryTypes(VkPhysicalDeviceMemoryProperties* mem_properties, uint32 tabs = 0)
{
    Print("    memory types:");
    for (uint32 i = 0; i < mem_properties->memoryTypeCount; ++i)
    {
        VkMemoryType* type = &mem_properties->memoryTypes[i];
        PrintLine();

        PrintTabs(tabs);
        PrintLine("VkMemoryType [%u]:", i);

        PrintTabs(tabs + 1);
        PrintLine("heapIndex: %u", type->heapIndex);

        PrintTabs(tabs + 1);
        PrintLine("propertyFlags:");

        PrintMemoryPropertyFlags(type->propertyFlags, tabs + 2);
    }
    PrintLine();
}

static void LogDeviceFeature(const char* feature_name, VkBool32 enabled, uint32 feature_index,
                             uint32 longest_feature_name_size)
{
    static constexpr const char* ALIGN_BUFFER = "................................................................";
    Print("            [%2u] %s", feature_index, feature_name);
    Print(" %.*s ", longest_feature_name_size - StringSize(feature_name), ALIGN_BUFFER);
    if (enabled == VK_TRUE)
    {
        PrintLine(OutputColor::GREEN, "%s", "TRUE");
    }
    else
    {
        PrintLine(OutputColor::RED, "%s", "FALSE");
    }
}

static void LogDeviceFeatures(DeviceFeatures* device_features)
{
    PrintLine("    device features:");

    // Vulkan 1.0 Features
    VkBool32* vulkan_1_0_device_features = GetDeviceFeaturesArray_1_0(&device_features->vulkan_1_0);
    PrintLine("        Vulkan 1.0:");
    for (uint32 i = 0; i < VULKAN_1_0_DEVICE_FEATURE_COUNT; ++i)
    {
        LogDeviceFeature(GetDeviceFeatureName_1_0(i), vulkan_1_0_device_features[i], i,
                         VULKAN_1_0_LONGEST_DEVICE_FEATURE_NAME_SIZE);
    }
    PrintLine();

    // Vulkan 1.1 Features
    VkBool32* vulkan_1_1_device_features = GetDeviceFeaturesArray_1_1(&device_features->vulkan_1_1);
    PrintLine("        Vulkan 1.1:");
    for (uint32 i = 0; i < VULKAN_1_1_DEVICE_FEATURE_COUNT; ++i)
    {
        LogDeviceFeature(GetDeviceFeatureName_1_1(i), vulkan_1_1_device_features[i], i,
                         VULKAN_1_1_LONGEST_DEVICE_FEATURE_NAME_SIZE);
    }
    PrintLine();

    // Vulkan 1.2 Features
    VkBool32* vulkan_1_2_device_features = GetDeviceFeaturesArray_1_2(&device_features->vulkan_1_2);
    PrintLine("        Vulkan 1.2:");
    for (uint32 i = 0; i < VULKAN_1_2_DEVICE_FEATURE_COUNT; ++i)
    {
        LogDeviceFeature(GetDeviceFeatureName_1_2(i), vulkan_1_2_device_features[i], i,
                         VULKAN_1_2_LONGEST_DEVICE_FEATURE_NAME_SIZE);
    }
    PrintLine();

    // Vulkan 1.3 Features
    VkBool32* vulkan_1_3_device_features = GetDeviceFeaturesArray_1_3(&device_features->vulkan_1_3);
    PrintLine("        Vulkan 1.3:");
    for (uint32 i = 0; i < VULKAN_1_3_DEVICE_FEATURE_COUNT; ++i)
    {
        LogDeviceFeature(GetDeviceFeatureName_1_3(i), vulkan_1_3_device_features[i], i,
                         VULKAN_1_3_LONGEST_DEVICE_FEATURE_NAME_SIZE);
    }
    PrintLine();
}

static void LogPhysicalDevice(PhysicalDevice* physical_device)
{
    VkPhysicalDeviceType type = physical_device->properties.deviceType;
    VkFormat depth_image_format = physical_device->depth_image_format;

    PrintLine("%s:", physical_device->properties.deviceName);
    PrintLine("    type: %s",
        type == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU   ? "VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU" :
        type == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ? "VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU" :
        "invalid");
    PrintLine("    depth_image_format: %s",
        depth_image_format == VK_FORMAT_D32_SFLOAT_S8_UINT ? "VK_FORMAT_D32_SFLOAT_S8_UINT" :
        depth_image_format == VK_FORMAT_D32_SFLOAT         ? "VK_FORMAT_D32_SFLOAT"         :
        depth_image_format == VK_FORMAT_D24_UNORM_S8_UINT  ? "VK_FORMAT_D24_UNORM_S8_UINT"  :
        depth_image_format == VK_FORMAT_D16_UNORM_S8_UINT  ? "VK_FORMAT_D16_UNORM_S8_UINT"  :
        depth_image_format == VK_FORMAT_D16_UNORM          ? "VK_FORMAT_D16_UNORM"          :
        "UNKNWON");
    PrintLine();
    PrintLine("    queue_families:");
    PrintLine("        graphics: %u", physical_device->queue_families.graphics);
    PrintLine("        present:  %u", physical_device->queue_families.present);
    PrintLine();
    LogMemoryTypes(&physical_device->mem_properties, 2);
    LogDeviceFeatures(&physical_device->features);
}

/// Internal
////////////////////////////////////////////////////////////
static QueueFamilies FindQueueFamilies(Stack* temp_stack, VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
    Stack frame = CreateFrame(temp_stack);

    QueueFamilies queue_families =
    {
        .graphics = UNSET_INDEX,
        .present  = UNSET_INDEX,
    };
    Array<VkQueueFamilyProperties> queue_family_properties = {};
    LoadVkQueueFamilyProperties(&queue_family_properties, &frame.allocator, physical_device);

    // Find first graphics queue family.
    for (uint32 queue_family_index = 0; queue_family_index < queue_family_properties.count; ++queue_family_index)
    {
        // Check if queue supports graphics operations.
        if (GetPtr(&queue_family_properties, queue_family_index)->queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            queue_families.graphics = queue_family_index;
            break;
        }
    }

    // Find first present queue family.
    for (uint32 queue_family_index = 0; queue_family_index < queue_family_properties.count; ++queue_family_index)
    {
        // Check if queue supports present operations.
        VkBool32 present_supported = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, queue_family_index, surface, &present_supported);
        if (present_supported)
        {
            queue_families.present = queue_family_index;
            break;
        }
    }

    return queue_families;
}

static VkFormat FindDepthImageFormat(VkPhysicalDevice physical_device)
{
    static constexpr VkFormat DEPTH_IMAGE_FORMATS[] =
    {
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM,
    };

    // Find format that supports depth-stencil attachment feature for physical device.
    for (uint32 i = 0; i < CTK_ARRAY_SIZE(DEPTH_IMAGE_FORMATS); i++)
    {
        VkFormat depth_image_format = DEPTH_IMAGE_FORMATS[i];
        VkFormatProperties format_properties = {};
        vkGetPhysicalDeviceFormatProperties(physical_device, depth_image_format, &format_properties);
        if (format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        {
            return depth_image_format;
        }
    }

    return VK_FORMAT_UNDEFINED;
}

static VkDeviceQueueCreateInfo GetSingleQueueInfo(uint32 queue_family_index)
{
    static constexpr float32 QUEUE_PRIORITIES[] = { 1.0f };
    VkDeviceQueueCreateInfo info =
    {
        .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .flags            = 0,
        .queueFamilyIndex = queue_family_index,
        .queueCount       = CTK_ARRAY_SIZE(QUEUE_PRIORITIES),
        .pQueuePriorities = QUEUE_PRIORITIES,
    };

    return info;
}

static void InitInstance(InstanceInfo* info, Stack* temp_stack)
{
    Stack frame = CreateFrame(temp_stack);
    VkResult res = VK_SUCCESS;

#ifdef RTK_ENABLE_VALIDATION
    VkDebugUtilsMessengerCreateInfoEXT debug_msgr_info =
    {
        .sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext           = NULL,
        .flags           = 0,
        .messageSeverity = info->debug_message_severity,
        .messageType     = info->debug_message_type,
        .pfnUserCallback = info->debug_callback,
        .pUserData       = NULL,
    };
#endif

    VkApplicationInfo app_info =
    {
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext              = NULL,
        .pApplicationName   = info->application_name,
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName        = info->application_name,
        .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion         = info->api_version,
    };

    static constexpr const char* REQUIRED_EXTENSIONS[] =
    {
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
        VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef RTK_ENABLE_VALIDATION
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
    };
    Array<const char*> extensions = {};
    InitArray(&extensions, &frame.allocator, CTK_ARRAY_SIZE(REQUIRED_EXTENSIONS) + info->extensions.count);
    PushRange(&extensions, REQUIRED_EXTENSIONS, CTK_ARRAY_SIZE(REQUIRED_EXTENSIONS));
    PushRange(&extensions, &info->extensions);

    Array<const char*> layers = {};
#ifdef RTK_ENABLE_VALIDATION
    InitArray(&layers, &frame.allocator, 1);
    Push(&layers, "VK_LAYER_KHRONOS_validation");
#endif

    VkInstanceCreateInfo create_info = {};
    create_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.flags                   = 0;
    create_info.pApplicationInfo        = &app_info;
    create_info.enabledExtensionCount   = extensions.count;
    create_info.ppEnabledExtensionNames = extensions.data;
#ifdef RTK_ENABLE_VALIDATION
    create_info.pNext                   = &debug_msgr_info;
    create_info.enabledLayerCount       = layers.count;
    create_info.ppEnabledLayerNames     = layers.data;
#else
    create_info.pNext                   = NULL;
    create_info.enabledLayerCount       = 0;
    create_info.ppEnabledLayerNames     = NULL;
#endif
    res = vkCreateInstance(&create_info, NULL, &g_context.instance);
    Validate(res, "vkCreateInstance() failed");

#ifdef RTK_ENABLE_VALIDATION
    RTK_LOAD_INSTANCE_EXTENSION_FUNCTION(g_context.instance, vkCreateDebugUtilsMessengerEXT);
    res = vkCreateDebugUtilsMessengerEXT(g_context.instance, &debug_msgr_info, NULL, &g_context.debug_messenger);
    Validate(res, "vkCreateDebugUtilsMessengerEXT() failed");
#endif
}

static void InitSurface()
{
    Window* window = GetWindow();
    VkWin32SurfaceCreateInfoKHR info =
    {
        .sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .hinstance = window->instance,
        .hwnd      = window->handle,
    };
    VkResult res = vkCreateWin32SurfaceKHR(g_context.instance, &info, NULL, &g_context.surface);
    Validate(res, "vkCreateWin32SurfaceKHR() failed");
}

static void LogUnsupportedDeviceFeature(PhysicalDevice* physical_device, const char* feature_name)
{
    PrintError("requested device feature \"%s\" not supported by device \"%s\"",
               feature_name, physical_device->properties.deviceName);
}

static void
LoadCapablePhysicalDevices(Stack* perm_stack, Stack* temp_stack, DeviceFeatures* enabled_features)
{
    Stack frame = CreateFrame(temp_stack);

    // Get system physical devices and ensure atleast 1 physical device was found.
    Array<VkPhysicalDevice> vk_physical_devices = {};
    LoadVkPhysicalDevices(&vk_physical_devices, &frame.allocator, g_context.instance);
    if (vk_physical_devices.count == 0)
    {
        CTK_FATAL("failed to find any physical devices");
    }

    // Initialize physical devices array to hold all physical devices if necessary.
    InitArray(&g_context.physical_devices, &perm_stack->allocator, vk_physical_devices.count);

    // Check all physical devices, and load the ones capable of rendering into the context's physical device list.
    for (uint32 physical_device_index = 0; physical_device_index < vk_physical_devices.count; ++physical_device_index)
    {
        VkPhysicalDevice vk_physical_device = Get(&vk_physical_devices, physical_device_index);

        // Collect all info about physical device.
        PhysicalDevice physical_device = {};
        physical_device.hnd                = vk_physical_device;
        physical_device.depth_image_format = FindDepthImageFormat(vk_physical_device);
        physical_device.queue_families     = FindQueueFamilies(&frame, vk_physical_device, g_context.surface);
        if (physical_device.queue_families.graphics != physical_device.queue_families.present)
        {
            physical_device.resource_sharing =
            {
                .mode                     = VK_SHARING_MODE_CONCURRENT,
                .queue_family_index_count = sizeof(QueueFamilies) / sizeof(uint32),
                .queue_family_indexes     = (const uint32*)&physical_device.queue_families,
            };
        }
        else
        {
            physical_device.resource_sharing =
            {
                .mode                     = VK_SHARING_MODE_EXCLUSIVE,
                .queue_family_index_count = 0,
                .queue_family_indexes     = NULL,
            };
        }

        vkGetPhysicalDeviceProperties(vk_physical_device, &physical_device.properties);
        vkGetPhysicalDeviceMemoryProperties(vk_physical_device, &physical_device.mem_properties);

        DeviceFeatures* device_features = &physical_device.features;
        InitDeviceFeatures(device_features);
        GetDeviceFeatures(physical_device.hnd, device_features);

        // Graphics and present queue families required.
        if (physical_device.queue_families.graphics == UNSET_INDEX ||
            physical_device.queue_families.present  == UNSET_INDEX)
        {
            continue;
        }

        // Depth image format required.
        if (physical_device.depth_image_format == VK_FORMAT_UNDEFINED)
        {
            continue;
        }

        // All requested enabled features must be supported.
        bool all_enabled_features_supported = true;

        // Vulkan 1.0 Features
        VkBool32* vulkan_1_0_enabled_features = GetDeviceFeaturesArray_1_0(&enabled_features->vulkan_1_0);
        VkBool32* vulkan_1_0_device_features = GetDeviceFeaturesArray_1_0(&device_features->vulkan_1_0);
        for (uint32 i = 0; i < VULKAN_1_0_DEVICE_FEATURE_COUNT; ++i)
        {
            if (vulkan_1_0_enabled_features[i] == VK_TRUE && vulkan_1_0_device_features[i] == VK_FALSE)
            {
                LogUnsupportedDeviceFeature(&physical_device, GetDeviceFeatureName_1_0(i));
                all_enabled_features_supported = false;
            }
        }

        // Vulkan 1.1 Features
        VkBool32* vulkan_1_1_enabled_features = GetDeviceFeaturesArray_1_1(&enabled_features->vulkan_1_1);
        VkBool32* vulkan_1_1_device_features = GetDeviceFeaturesArray_1_1(&device_features->vulkan_1_1);
        for (uint32 i = 0; i < VULKAN_1_1_DEVICE_FEATURE_COUNT; ++i)
        {
            if (vulkan_1_1_enabled_features[i] == VK_TRUE && vulkan_1_1_device_features[i] == VK_FALSE)
            {
                LogUnsupportedDeviceFeature(&physical_device, GetDeviceFeatureName_1_1(i));
                all_enabled_features_supported = false;
            }
        }

        // Vulkan 1.2 Features
        VkBool32* vulkan_1_2_enabled_features = GetDeviceFeaturesArray_1_2(&enabled_features->vulkan_1_2);
        VkBool32* vulkan_1_2_device_features = GetDeviceFeaturesArray_1_2(&device_features->vulkan_1_2);
        for (uint32 i = 0; i < VULKAN_1_2_DEVICE_FEATURE_COUNT; ++i)
        {
            if (vulkan_1_2_enabled_features[i] == VK_TRUE && vulkan_1_2_device_features[i] == VK_FALSE)
            {
                LogUnsupportedDeviceFeature(&physical_device, GetDeviceFeatureName_1_2(i));
                all_enabled_features_supported = false;
            }
        }

        // Vulkan 1.3 Features
        VkBool32* vulkan_1_3_enabled_features = GetDeviceFeaturesArray_1_3(&enabled_features->vulkan_1_3);
        VkBool32* vulkan_1_3_device_features = GetDeviceFeaturesArray_1_3(&device_features->vulkan_1_3);
        for (uint32 i = 0; i < VULKAN_1_3_DEVICE_FEATURE_COUNT; ++i)
        {
            if (vulkan_1_3_enabled_features[i] == VK_TRUE && vulkan_1_3_device_features[i] == VK_FALSE)
            {
                LogUnsupportedDeviceFeature(&physical_device, GetDeviceFeatureName_1_3(i));
                all_enabled_features_supported = false;
            }
        }

        if (!all_enabled_features_supported)
        {
            continue;
        }

        // Physical device is capable.
        Push(&g_context.physical_devices, physical_device);
    }

    // Ensure atleast 1 capable physical device was loaded.
    if (g_context.physical_devices.count == 0)
    {
        CTK_FATAL("failed to find any capable physical devices");
    }
}

static void UsePhysicalDevice(uint32 index)
{
    if (index >= g_context.physical_devices.count)
    {
        CTK_FATAL("physical device index %u is out of bounds: max is %u", index, g_context.physical_devices.count - 1);
    }

    g_context.physical_device = GetPtr(&g_context.physical_devices, index);
}

static void InitDevice(DeviceFeatures* enabled_features)
{
    QueueFamilies* queue_families = &g_context.physical_device->queue_families;

    // Add queue creation info for 1 queue in each queue family.
    FArray<VkDeviceQueueCreateInfo, 2> queue_infos = {};
    Push(&queue_infos, GetSingleQueueInfo(queue_families->graphics));

    // Don't create separate queues if present and graphics queue families are the same.
    if (queue_families->present != queue_families->graphics)
    {
        Push(&queue_infos, GetSingleQueueInfo(queue_families->present));
    }

    // Create device, specifying enabled extensions and features.
    const char* enabled_extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    VkDeviceCreateInfo create_info =
    {
        .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext                   = &enabled_features->vulkan_1_0,
        .flags                   = 0,
        .queueCreateInfoCount    = queue_infos.count,
        .pQueueCreateInfos       = queue_infos.data,
        .enabledLayerCount       = 0,
        .ppEnabledLayerNames     = NULL,
        .enabledExtensionCount   = CTK_ARRAY_SIZE(enabled_extensions),
        .ppEnabledExtensionNames = enabled_extensions,
        .pEnabledFeatures        = NULL,
    };
    VkResult res = vkCreateDevice(g_context.physical_device->hnd, &create_info, NULL, &g_context.device);
    Validate(res, "vkCreateDevice() failed");
}

static void InitQueues()
{
    QueueFamilies* queue_families = &g_context.physical_device->queue_families;
    vkGetDeviceQueue(g_context.device, queue_families->graphics, 0, &g_context.graphics_queue);
    vkGetDeviceQueue(g_context.device, queue_families->present, 0, &g_context.present_queue);
}

static void InitMainCommandState()
{
    VkDevice device = g_context.device;
    VkResult res = VK_SUCCESS;

    // Main Command Pool
    VkCommandPoolCreateInfo pool_info =
    {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = g_context.physical_device->queue_families.graphics,
    };
    res = vkCreateCommandPool(device, &pool_info, NULL, &g_context.main_command_pool);
    Validate(res, "vkCreateCommandPool() failed");

    // Temp Command Buffer
    VkCommandBufferAllocateInfo allocate_info =
    {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = g_context.main_command_pool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    res = vkAllocateCommandBuffers(device, &allocate_info, &g_context.temp_command_buffer);
    Validate(res, "vkAllocateCommandBuffers() failed");
}


static void CreateSwapchain(Stack* temp_stack, FreeList* free_list)
{
    Stack frame = CreateFrame(temp_stack);

    Swapchain* swapchain = &g_context.swapchain;
    VkDevice device = g_context.device;
    VkResult res = VK_SUCCESS;

    // Create swapchain.
    VkSwapchainCreateInfoKHR info =
    {
        .sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .flags                 = 0,
        .surface               = g_context.surface,
        .minImageCount         = swapchain->surface_min_image_count,
        .imageFormat           = swapchain->surface_format.format,
        .imageColorSpace       = swapchain->surface_format.colorSpace,
        .imageExtent           = swapchain->surface_extent,
        .imageArrayLayers      = 1, // Always 1 for standard image
        .imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                 VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode      = swapchain->image_sharing_mode,
        .queueFamilyIndexCount = swapchain->queue_family_index_count,
        .pQueueFamilyIndices   = swapchain->queue_family_indexes,
        .preTransform          = swapchain->surface_transform,
        .compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode           = swapchain->surface_present_mode,
        .clipped               = VK_TRUE,
        .oldSwapchain          = VK_NULL_HANDLE,
    };
    res = vkCreateSwapchainKHR(device, &info, NULL, &swapchain->hnd);
    Validate(res, "vkCreateSwapchainKHR() failed");

    // Create swapchain image views.
    Array<VkImage> swapchain_images = {};
    LoadVkSwapchainImages(&swapchain_images, &frame.allocator, device, swapchain->hnd);
    InitArrayFull(&swapchain->image_views, &free_list->allocator, swapchain_images.count);
    for (uint32 i = 0; i < swapchain_images.count; ++i)
    {
        VkImageViewCreateInfo view_info =
        {
            .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .flags    = 0,
            .image    = Get(&swapchain_images, i),
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format   = swapchain->surface_format.format,
            .components =
            {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange =
            {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0,
                .levelCount     = VK_REMAINING_MIP_LEVELS,
                .baseArrayLayer = 0,
                .layerCount     = VK_REMAINING_ARRAY_LAYERS,
            },
        };
        res = vkCreateImageView(device, &view_info, NULL, GetPtr(&swapchain->image_views, i));
        Validate(res, "vkCreateImageView() failed");
    }
}

static void InitSwapchain(Stack* temp_stack, FreeList* free_list)
{
    Stack frame = CreateFrame(temp_stack);

    Swapchain* swapchain = &g_context.swapchain;
    VkSurfaceKHR surface = g_context.surface;
    PhysicalDevice* physical_device = g_context.physical_device;

    /// Configure Swapchain
    ////////////////////////////////////////////////////////////

    // Get surface info.
    Array<VkSurfaceFormatKHR> formats = {};
    Array<VkPresentModeKHR> present_modes = {};
    LoadVkSurfaceFormats(&formats, &frame.allocator, physical_device->hnd, surface);
    LoadVkSurfacePresentModes(&present_modes, &frame.allocator, physical_device->hnd, surface);
    VkSurfaceCapabilitiesKHR surface_capabilities = {};
    GetSurfaceCapabilities(&surface_capabilities);

    // Default to first surface format, then check for preferred 4-component 8-bit BGRA unnormalized format and sRG
    // color space.
    swapchain->surface_format = Get(&formats, 0);
    for (uint32 i = 0; i < formats.count; ++i)
    {
        VkSurfaceFormatKHR surface_format = Get(&formats, i);
        if (surface_format.format == VK_FORMAT_R8G8B8A8_UNORM &&
            surface_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            swapchain->surface_format = surface_format;
            break;
        }
    }

    // Default to FIFO (only present mode with guarenteed availability), check for preferred mailbox present mode.
    swapchain->surface_present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (uint32 i = 0; i < present_modes.count; ++i)
    {
        VkPresentModeKHR surface_present_mode = Get(&present_modes, i);
        if (surface_present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            swapchain->surface_present_mode = surface_present_mode;
            break;
        }
    }

    // Set surface extent and transform.
    swapchain->surface_extent    = surface_capabilities.currentExtent;
    swapchain->surface_transform = surface_capabilities.currentTransform;

    // Set image count to min image count + 1 or max image count (whichever is smaller).
    swapchain->surface_min_image_count = surface_capabilities.minImageCount + 1;
    if (surface_capabilities.maxImageCount > 0 && swapchain->surface_min_image_count > surface_capabilities.maxImageCount)
    {
        swapchain->surface_min_image_count = surface_capabilities.maxImageCount;
    }

    // Check if image sharing mode needs to be concurrent due to separate graphics & present queue families.
    QueueFamilies* queue_families = &g_context.physical_device->queue_families;
    if (queue_families->graphics != queue_families->present)
    {
        swapchain->image_sharing_mode       = VK_SHARING_MODE_CONCURRENT;
        swapchain->queue_family_index_count = sizeof(QueueFamilies) / sizeof(uint32);
        swapchain->queue_family_indexes     = (uint32*)queue_families;
    }
    else
    {
        swapchain->image_sharing_mode       = VK_SHARING_MODE_EXCLUSIVE;
        swapchain->queue_family_index_count = 0;
        swapchain->queue_family_indexes     = NULL;
    }

    /// Create Swapchain
    ////////////////////////////////////////////////////////////
    CreateSwapchain(&frame, free_list);
}

static void InitRenderCommandPools(Stack* perm_stack)
{
    InitArray(&g_context.render_command_pools, &perm_stack->allocator, g_context.render_thread_count);
    VkCommandPoolCreateInfo info =
    {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = g_context.physical_device->queue_families.graphics,
    };
    for (uint32 i = 0; i < g_context.render_thread_count; ++i)
    {
        VkResult res = vkCreateCommandPool(g_context.device, &info, NULL, Push(&g_context.render_command_pools));
        Validate(res, "vkCreateCommandPool() failed");
    }
}

static VkFence CreateFence(VkDevice device)
{
    VkFenceCreateInfo info =
    {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    VkFence fence = VK_NULL_HANDLE;
    VkResult res = vkCreateFence(device, &info, NULL, &fence);
    Validate(res, "vkCreateFence() failed");

    return fence;
}

static VkSemaphore CreateSemaphore(VkDevice device, VkSemaphoreType type)
{
    VkSemaphoreTypeCreateInfo type_info =
    {
        .sType         = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
        .pNext         = NULL,
        .semaphoreType = type,
        .initialValue  = 0,
    };
    VkSemaphoreCreateInfo info =
    {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = &type_info,
        .flags = 0,
    };
    VkSemaphore semaphore = VK_NULL_HANDLE;
    VkResult res = vkCreateSemaphore(device, &info, NULL, &semaphore);
    Validate(res, "vkCreateSemaphore() failed");

    return semaphore;
}

static void InitFrames(Stack* perm_stack)
{
    VkResult res = VK_SUCCESS;
    VkDevice device = g_context.device;

    uint32 frame_count = g_context.swapchain.image_views.count + 1;
    CTK_ASSERT(frame_count <= MAX_FRAME_COUNT);

    InitRingBuffer(&g_context.frames, &perm_stack->allocator, frame_count);
    CTK_ITER(frame, &g_context.frames)
    {
        // Sync State
        frame->image_acquired           = CreateSemaphore(device, VK_SEMAPHORE_TYPE_BINARY);
        frame->render_finished          = CreateSemaphore(device, VK_SEMAPHORE_TYPE_BINARY);
        frame->command_buffers_complete = CreateFence(device);

        // primary_render_command_buffer
        {
            VkCommandBufferAllocateInfo allocate_info =
            {
                .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool        = g_context.main_command_pool,
                .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 1,
            };
            res = vkAllocateCommandBuffers(device, &allocate_info, &frame->primary_render_command_buffer);
            Validate(res, "vkAllocateCommandBuffers() failed");
        }

        // render_command_buffers
        InitArray(&frame->render_command_buffers, &perm_stack->allocator, g_context.render_command_pools.count);
        for (uint32 i = 0; i < g_context.render_command_pools.count; ++i)
        {
            VkCommandBufferAllocateInfo allocate_info =
            {
                .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool        = Get(&g_context.render_command_pools, i),
                .level              = VK_COMMAND_BUFFER_LEVEL_SECONDARY,
                .commandBufferCount = 1,
            };
            res = vkAllocateCommandBuffers(device, &allocate_info, Push(&frame->render_command_buffers));
            Validate(res, "vkAllocateCommandBuffers() failed");
        }
    }
}

/// Interface
////////////////////////////////////////////////////////////
static void InitContext(Stack* perm_stack, Stack* temp_stack, FreeList* free_list, ContextInfo* info)
{
    g_context.render_thread_count = info->render_thread_count;

    InitInstance(&info->instance_info, temp_stack);
    InitSurface();

    // Load capable physical devices and select the first one.
    LoadCapablePhysicalDevices(perm_stack, temp_stack, &info->enabled_features);
    UsePhysicalDevice(0);

    // Initialize device state.
    InitDevice(&info->enabled_features);
    InitQueues();
    InitMainCommandState();

    // Initialize rendering state.
    InitSwapchain(temp_stack, free_list);
    InitRenderCommandPools(perm_stack);
    InitFrames(perm_stack);
};

static VkDevice GetDevice()
{
    CTK_ASSERT(g_context.device != VK_NULL_HANDLE);
    return g_context.device;
}

static PhysicalDevice* GetPhysicalDevice()
{
    CTK_ASSERT(g_context.physical_device != NULL);
    return g_context.physical_device;
}

static Swapchain* GetSwapchain()
{
    CTK_ASSERT(g_context.swapchain.hnd != VK_NULL_HANDLE);
    return &g_context.swapchain;
}

static VkCommandBuffer GetTempCommandBuffer()
{
    CTK_ASSERT(g_context.temp_command_buffer != VK_NULL_HANDLE);
    return g_context.temp_command_buffer;
}

static VkQueue GetGraphicsQueue()
{
    CTK_ASSERT(g_context.graphics_queue != VK_NULL_HANDLE);
    return g_context.graphics_queue;
}

static VkQueue GetPresentQueue()
{
    CTK_ASSERT(g_context.present_queue != VK_NULL_HANDLE);
    return g_context.present_queue;
}

static uint32 GetFrameCount()
{
    return g_context.frames.size;
}

static uint32 GetFrameIndex()
{
    return g_context.frames.index;
}

static Frame* GetCurrentFrame()
{
    return GetCurrentPtr(&g_context.frames);
}

static uint32 GetRenderThreadCount()
{
    return g_context.render_thread_count;
}

static void GetSurfaceCapabilities(VkSurfaceCapabilitiesKHR* capabilities)
{
    VkResult res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g_context.physical_device->hnd, g_context.surface,
                                                             capabilities);
    Validate(res, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR() failed");
}

static void BeginTempCommandBuffer()
{
    VkCommandBufferBeginInfo info =
    {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext            = NULL,
        .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = NULL,
    };
    vkBeginCommandBuffer(g_context.temp_command_buffer, &info);
}

static void SubmitTempCommandBuffer()
{
    vkEndCommandBuffer(g_context.temp_command_buffer);

    VkSubmitInfo submit_info =
    {
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext                = NULL,
        .waitSemaphoreCount   = 0,
        .pWaitSemaphores      = NULL,
        .pWaitDstStageMask    = NULL,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &g_context.temp_command_buffer,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores    = NULL,
    };
    VkResult res = vkQueueSubmit(g_context.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    Validate(res, "vkQueueSubmit() failed");

    vkQueueWaitIdle(g_context.graphics_queue);
}

static void UpdateSwapchainSurfaceExtent(Stack* temp_stack, FreeList* free_list)
{
    Swapchain* swapchain = &g_context.swapchain;

    // Destroy swapchain image views.
    for (uint32 i = 0; i < swapchain->image_views.count; ++i)
    {
        vkDestroyImageView(g_context.device, Get(&swapchain->image_views, i), NULL);
    }
    DeinitArray(&swapchain->image_views, &free_list->allocator);

    // Update swapchain surface extent.
    VkSurfaceCapabilitiesKHR surface_capabilities = {};
    GetSurfaceCapabilities(&surface_capabilities);
    g_context.swapchain.surface_extent = surface_capabilities.currentExtent;

    // Recreate swapchain.
    vkDestroySwapchainKHR(g_context.device, swapchain->hnd, NULL);
    CreateSwapchain(temp_stack, free_list);
}

static void WaitIdle()
{
    vkDeviceWaitIdle(g_context.device);
}

static void NextFrame()
{
    Next(&g_context.frames);
}
