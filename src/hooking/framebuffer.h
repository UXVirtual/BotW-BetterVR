#pragma once

#include "rendering/texture.h"

struct CaptureTexture {
    bool initialized;
    VkExtent2D foundSize;
    VkImage foundImage;
    VkFormat format;
    VkExtent2D minSize;
    std::unique_ptr<SharedTexture> sharedTexture;
    //std::unique_ptr<Texture> d3d12Texture;

    // current frame state
    std::atomic<VkCommandBuffer> captureCmdBuffer = VK_NULL_HANDLE;
};

extern std::array<CaptureTexture, 1> captureTextures;