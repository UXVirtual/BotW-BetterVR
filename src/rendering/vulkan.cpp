#include "vulkan.h"
#include "hooking/layer.h"
#include "instance.h"

RND_Vulkan::RND_Vulkan(VkInstance vkInstance, VkPhysicalDevice vkPhysDevice, VkDevice vkDevice): m_instance(vkInstance), m_physicalDevice(vkPhysDevice), m_device(vkDevice) {
    m_instanceDispatch = vkroots::tables::LookupInstanceDispatch(vkInstance);
    m_physicalDeviceDispatch = vkroots::tables::LookupPhysicalDeviceDispatch(vkPhysDevice);
    m_deviceDispatch = vkroots::tables::LookupDeviceDispatch(vkDevice);

    m_physicalDeviceDispatch->GetPhysicalDeviceMemoryProperties2KHR(vkPhysDevice, &m_memoryProperties);
}

RND_Vulkan::~RND_Vulkan() {
}

uint32_t RND_Vulkan::FindMemoryType(uint32_t memoryTypeBitsRequirement, VkMemoryPropertyFlags requirementsMask) {
    for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++) {
        const uint32_t memoryTypeBits = (1 << i);
        const bool isRequiredMemoryType = memoryTypeBitsRequirement & memoryTypeBits;
        const bool satisfiesFlags = (m_memoryProperties.memoryProperties.memoryTypes[i].propertyFlags & requirementsMask) == requirementsMask;

        if (isRequiredMemoryType && satisfiesFlags) {
            return i;
        }
    }
    checkAssert(false, "Failed to find suitable memory type");
    return 0;
}


VkResult VRLayer::VkDeviceOverrides::CreateSwapchainKHR(const vkroots::VkDeviceDispatch* pDispatch, VkDevice device, const VkSwapchainCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchain) {
    if (!VRManager::instance().VK) {
        VRManager::instance().Init(pDispatch->pPhysicalDeviceDispatch->Instance, pDispatch->PhysicalDevice, device);
        VRManager::instance().InitSession();
    }

    return pDispatch->CreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);
}