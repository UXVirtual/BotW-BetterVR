#pragma once

class RND_Vulkan {
public:
    RND_Vulkan(VkInstance vkInstance, VkPhysicalDevice vkPhysDevice, VkDevice vkDevice);
    ~RND_Vulkan();

    uint32_t FindMemoryType(uint32_t memoryTypeBitsRequirement, VkMemoryPropertyFlags requirementsMask);

    VkDevice GetDevice() { return m_device; }

    const vkroots::VkInstanceDispatch* GetInstanceDispatch() { return m_instanceDispatch; }

    const vkroots::VkPhysicalDeviceDispatch* GetPhysicalDeviceDispatch() { return m_physicalDeviceDispatch; }

    const vkroots::VkDeviceDispatch* GetDeviceDispatch() { return m_deviceDispatch; }

private:
    VkInstance m_instance;
    VkPhysicalDevice m_physicalDevice;
    VkDevice m_device;

    VkPhysicalDeviceMemoryProperties2 m_memoryProperties = {};

    // todo: use these with caution
    const vkroots::VkInstanceDispatch* m_instanceDispatch;
    const vkroots::VkPhysicalDeviceDispatch* m_physicalDeviceDispatch;
    const vkroots::VkDeviceDispatch* m_deviceDispatch;
};