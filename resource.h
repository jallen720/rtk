/// Data
////////////////////////////////////////////////////////////
struct ResourceMemory
{
    VkDeviceSize          size;
    VkDeviceMemory        device_mem;
    uint8*                host_mem;
    VkMemoryPropertyFlags mem_properties;
};
