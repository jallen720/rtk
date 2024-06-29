/// Interface
////////////////////////////////////////////////////////////
template<typename Type, typename VkFunc, typename... Args>
static void LoadVkArray(Array<Type>* array, Allocator* allocator, VkFunc vk_func, Args... args)
{
    VkResult res = VK_SUCCESS;

    // Get number of elements for array, returning without initializing array if none were found.
    uint32 count = 0;
    res = vk_func(args..., &count, NULL);
    Validate(res, "failed to get vulkan array count");
    if (count == 0)
    {
        return;
    }

    // Initialize and fill array.
    *array = CreateArrayFull<Type>(allocator, count);
    res = vk_func(args..., &array->count, array->data);
    Validate(res, "failed to get vulkan array values");
}

template<typename Type, typename VkFunc, typename... Args>
static void LoadVkArrayUnchecked(Array<Type>* array, Allocator* allocator, VkFunc vk_func, Args... args)
{
    // Get number of elements for array, returning without initializing array if none were found.
    uint32 count = 0;
    vk_func(args..., &count, NULL);
    if (count == 0)
    {
        return;
    }

    // Initialize and fill array.
    *array = CreateArrayFull<Type>(allocator, count);
    vk_func(args..., &array->count, array->data);
}

static void LoadVkSwapchainImages(Array<VkImage>* array, Allocator* allocator, VkDevice device,
                                  VkSwapchainKHR swapchain)
{
    LoadVkArray(array, allocator, vkGetSwapchainImagesKHR, device, swapchain);
}

static void LoadVkQueueFamilyProperties(Array<VkQueueFamilyProperties>* array, Allocator* allocator,
                                        VkPhysicalDevice physical_device)
{
    LoadVkArrayUnchecked(array, allocator, vkGetPhysicalDeviceQueueFamilyProperties, physical_device);
}

static void LoadVkPhysicalDevices(Array<VkPhysicalDevice>* array, Allocator* allocator, VkInstance instance)
{
    LoadVkArray(array, allocator, vkEnumeratePhysicalDevices, instance);
}

static void LoadVkSurfaceFormats(Array<VkSurfaceFormatKHR>* array, Allocator* allocator,
                                 VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
    LoadVkArray(array, allocator, vkGetPhysicalDeviceSurfaceFormatsKHR, physical_device, surface);
}

static void LoadVkSurfacePresentModes(Array<VkPresentModeKHR>* array, Allocator* allocator,
                                      VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
    LoadVkArray(array, allocator, vkGetPhysicalDeviceSurfacePresentModesKHR, physical_device, surface);
}
