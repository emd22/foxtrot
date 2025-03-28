#include "Framebuffer.hpp"
#include "Core/Defines.hpp"
#include "Renderer/Backend/RenderPanic.hpp"
#include "Renderer/Backend/Vulkan/Device.hpp"
#include "Renderer/Renderer.hpp"
#include <Core/Types.hpp>
#include "vulkan/vulkan_core.h"

namespace vulkan {

AR_SET_MODULE_NAME("Framebuffer")

void Framebuffer::Create(StaticArray<VkImageView> &image_views, GraphicsPipeline &pipeline, Vec2i size)
{
    AssertRendererExists();

    const VkFramebufferCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = pipeline.RenderPass.RenderPass,
        .attachmentCount = (uint32)image_views.Size,
        .pAttachments = image_views.Data,
        .width = (uint32)size.Width(),
        .height = (uint32)size.Height(),
        .layers = 1,
    };

    this->mDevice = RendererState->Vulkan.GetDevice();

    const VkResult status = vkCreateFramebuffer(this->mDevice->Device, &create_info, nullptr, &this->Framebuffer);

    if (status != VK_SUCCESS) {
        Panic("Failed to create framebuffer", status);
    }
}

void Framebuffer::Destroy()
{
    vkDestroyFramebuffer(this->mDevice->Device, this->Framebuffer, nullptr);
}


}; // namespace vulkan
