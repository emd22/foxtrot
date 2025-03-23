#pragma once

#include <vulkan/vulkan.h>

#include <Core/StaticArray.hpp>
#include <Math/Vector.hpp>

class GPUDevice;

class Swapchain {
public:
    Swapchain() = default;

    void Init(Vec2i size, VkSurfaceKHR &surface, GPUDevice &device);

    ~Swapchain();

private:
    void CreateSwapchain(Vec2i size, VkSurfaceKHR &surface, GPUDevice &device);
    void CreateSwapchainImages(GPUDevice &device);

    void CreateImageViews(GPUDevice &device);
    // void CreateSwapchainFramebuffers();

public:

    StaticArray<VkImageView> ImageViews;
    StaticArray<VkImage> Images;

    Vec2i Extent = Vec2i::Zero();

    VkSurfaceFormatKHR SurfaceFormat;

    bool Initialized = false;

private:
    VkSwapchainKHR mSwapchain = nullptr;
};
