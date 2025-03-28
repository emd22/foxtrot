#include "Swapchain.hpp"
#include "Core/Defines.hpp"
#include "Device.hpp"

#include <Core/FxPanic.hpp>

#include "Renderer/Backend/Vulkan/Pipeline.hpp"
#include "Renderer/Renderer.hpp"
#include "vulkan/vulkan_core.h"

#include <vulkan/vulkan.h>

AR_SET_MODULE_NAME("Swapchain")

namespace vulkan {

void Swapchain::Init(Vec2i size, VkSurfaceKHR &surface, GPUDevice *device)
{
    AssertRendererExists();

    this->mDevice = device;

    this->CreateSwapchain(size, surface);
    this->CreateSwapchainImages();
    this->CreateImageViews();

    this->Initialized = true;
}

void Swapchain::CreateSwapchainImages()
{
    uint32 image_count;

    vkGetSwapchainImagesKHR(this->mDevice->Device, this->mSwapchain, &image_count, nullptr);

    this->Images.InitSize(image_count);
    vkGetSwapchainImagesKHR(this->mDevice->Device, this->mSwapchain, &image_count, this->Images.Data);
}

void Swapchain::CreateImageViews()
{
    const uint64 images_size = this->Images.Size;

    this->ImageViews.InitSize(images_size);

    for (int32 i = 0; i < images_size; i++) {
        const VkImageViewCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = this->Images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = this->SurfaceFormat.format,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            }
        };

        const VkResult status = vkCreateImageView(this->mDevice->Device, &create_info, nullptr, &this->ImageViews[i]);
        if (status != VK_SUCCESS) {
            FxPanic("Could not create swapchain image view", status);
        }
    }
}

void Swapchain::CreateSwapchain(Vec2i size, VkSurfaceKHR &surface)
{
    this->Extent = size;

    VkSurfaceCapabilitiesKHR capabilities;
    const VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(this->mDevice->Physical, surface, &capabilities);

    if (result != VK_SUCCESS) {
        FxPanic("Error retrieving surface capabilities", result);
    }

    const VkExtent2D extent = {
        .width = (uint32)size.GetX(),
        .height = (uint32)size.GetY(),
    };

    uint32 image_count = capabilities.minImageCount + 1;

    if (capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount) {
        image_count = capabilities.maxImageCount;
    }

    Log::Info("Swapchain - Min:%d, Max:%d, Selected:%d", capabilities.minImageCount, capabilities.maxImageCount, image_count);

    this->SurfaceFormat = this->mDevice->GetBestSurfaceFormat();

    const VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;

    VkSwapchainCreateInfoKHR create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,

        .minImageCount = image_count,

        .imageFormat = this->SurfaceFormat.format,
        .imageColorSpace = this->SurfaceFormat.colorSpace,

        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,

        .presentMode = present_mode,
        .preTransform = capabilities.currentTransform,

        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .clipped = VK_TRUE,
        // mSwapchain is null if not initialized
        .oldSwapchain = this->mSwapchain,
    };

    if (this->mDevice->mQueueFamilies.IsGraphicsAlsoPresent()) {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = nullptr;
    }
    else {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;

        const auto indices = this->mDevice->mQueueFamilies.GetQueueIndexList();
        create_info.pQueueFamilyIndices = indices.data();
    }

    const VkResult status = vkCreateSwapchainKHR(this->mDevice->Device, &create_info, nullptr, &this->mSwapchain);

    if (status != VK_SUCCESS) {
        FxPanic("Could not create swapchain", status);
    }
}

void Swapchain::CreateSwapchainFramebuffers(GraphicsPipeline *pipeline)
{
    Log::Debug("Image view count: %d", this->ImageViews.Size);
    this->Framebuffers.Free();
    this->Framebuffers.InitSize(this->ImageViews.Size);

    StaticArray<VkImageView> temp_views;
    temp_views.InitSize(1);

    for (int i = 0; i < this->ImageViews.Size; i++) {
        temp_views[0] = this->ImageViews[i];

        this->Framebuffers[i].Create(temp_views, *pipeline, this->Extent);
    }

    Log::Debug("Create framebuffers", 0);

    this->mPipeline = pipeline;
}

void Swapchain::DestroyFramebuffersAndImageViews()
{
    for (int i = 0; i < this->ImageViews.Size; i++) {
        this->Framebuffers[i].Destroy();
        vkDestroyImageView(this->mDevice->Device, this->ImageViews[i], nullptr);
    }

    this->Framebuffers.Free();
    this->ImageViews.Free();
}

void Swapchain::DestroyInternalSwapchain()
{
    vkDestroySwapchainKHR(this->mDevice->Device, this->mSwapchain, nullptr);
}

void Swapchain::Destroy()
{
    this->DestroyFramebuffersAndImageViews();
    this->Images.Free();
    this->DestroyInternalSwapchain();

    this->Initialized = false;
}

Swapchain::~Swapchain()
{
    this->Images.Free();
}

} // namespace vulkan
