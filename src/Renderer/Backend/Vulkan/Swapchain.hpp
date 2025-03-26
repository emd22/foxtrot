#pragma once

#include "Renderer/Backend/Vulkan/Pipeline.hpp"
#include "Renderer/Backend/Vulkan/Pipeline.hpp"
#include <vulkan/vulkan.h>

#include "Framebuffer.hpp"

#include <Core/StaticArray.hpp>
#include <Math/Vector.hpp>

class GPUDevice;

namespace vulkan {

class Swapchain {
public:
    Swapchain() = default;

    void Init(Vec2i size, VkSurfaceKHR &surface, GPUDevice &device);
    void CreateSwapchainFramebuffers(GraphicsPipeline *pipeline);

    ~Swapchain();

private:
    void CreateSwapchain(Vec2i size, VkSurfaceKHR &surface, GPUDevice &device);
    void CreateSwapchainImages(GPUDevice &device);

    void CreateImageViews(GPUDevice &device);

public:
    StaticArray<VkImageView> ImageViews;
    StaticArray<VkImage> Images;
    StaticArray<Framebuffer> Framebuffers;

    Vec2i Extent = Vec2i::Zero();

    VkSurfaceFormatKHR SurfaceFormat;

    bool Initialized = false;

private:
    GraphicsPipeline *mPipeline = nullptr;
    VkSwapchainKHR mSwapchain = nullptr;
};

}; // namespace vulkan
