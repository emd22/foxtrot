#pragma once

#include "RenderBackend.hpp"
#include "ThirdParty/vk_mem_alloc.h"
#include "vulkan/vulkan_core.h"

#include "Vulkan/Swapchain.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/FrameData.hpp"

#include <vulkan/vulkan.hpp>

struct SDL_Window;

namespace vulkan {

enum class FrameResult
{
    Success,
    GraphicsOutOfDate,
    RenderError,
};

class VkRenderBackend final : RenderBackend {
public:
    const uint32_t FramesInFlight = 2;

    VkRenderBackend() = default;
    using ExtensionList = std::vector<VkExtensionProperties>;
    using ExtensionNames = std::vector<const char *>;

    void Init(Vec2i window_size);
    void Destroy();

    FrameResult BeginFrame(GraphicsPipeline &pipeline);
    void FinishFrame(GraphicsPipeline &pipeline);

    void SelectWindow(SDL_Window *window)
    {
        this->mWindow = window;
    }

    vulkan::GPUDevice *GetDevice()
    {
        return &this->mDevice;
    }

    FrameData *GetFrame();

    uint32 GetFrameNumber() { return this->mFrameNumber; }
    uint32 GetImageIndex() { return this->mImageIndex; }
    VmaAllocator *GetGPUAllocator() { return &this->GPUAllocator; }

private:
    void InitVulkan();
    void CreateSurfaceFromWindow();
    void InitGPUAllocator();

    void SubmitFrame();
    void PresentFrame();

    void InitFrames();
    void DestroyFrames();

    FrameResult GetNextSwapchainImage(FrameData *frame);

    ExtensionList QueryInstanceExtensions(bool invalidate_previous = false);
    ExtensionNames MakeInstanceExtensionList(ExtensionNames user_requested_extensions);
    ExtensionNames CheckExtensionsAvailable(ExtensionNames requested_extensions);

    std::vector<VkLayerProperties> GetAvailableValidationLayers();

public:
    vulkan::Swapchain Swapchain;
    StaticArray<FrameData> Frames;

    bool Initialized = false;

    VmaAllocator GPUAllocator = nullptr;
private:

    VkInstance mInstance = nullptr;
    VkSurfaceKHR mWindowSurface = nullptr;

    SDL_Window *mWindow = nullptr;
    vulkan::GPUDevice mDevice;


    VkDebugUtilsMessengerEXT mDebugMessenger;

    ExtensionList mAvailableExtensions;

    uint32 mFrameNumber = 0;
    uint32 mImageIndex = 0;
};

}; // namespace vulkan
