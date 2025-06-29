#pragma once

#include "RvkPipeline.hpp"
#include "RvkImage.hpp"

#include <Core/FxSizedArray.hpp>
#include <Math/Vector.hpp>

#include <vulkan/vulkan.h>

class RvkGpuDevice;

class RvkSwapchain {
public:
    RvkSwapchain() = default;
    ~RvkSwapchain();

    void Init(Vec2u size, VkSurfaceKHR &surface, RvkGpuDevice *device);
    void Destroy();

    VkSwapchainKHR GetSwapchain() { return mSwapchain; }

private:
    void CreateSwapchain(Vec2u size, VkSurfaceKHR &surface);
    void CreateSwapchainImages();

    void CreateImageViews();
    void DestroyImageViews();

    void DestroyInternalSwapchain();

public:
    FxSizedArray<VkImageView> ImageViews;
    FxSizedArray<VkImage> Images;

    FxSizedArray<RvkImage> DepthImages;

    Vec2u Extent = Vec2u::Zero;

    VkSurfaceFormatKHR SurfaceFormat;

    bool Initialized = false;

private:
    RvkGpuDevice *mDevice = nullptr;
    VkSwapchainKHR mSwapchain = nullptr;
};
