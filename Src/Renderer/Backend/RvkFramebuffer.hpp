#pragma once

#include <vulkan/vulkan.h>

#include "RvkRenderPass.hpp"

#include "Math/Vec2.hpp"
#include "RvkPipeline.hpp"

#include <Core/FxSizedArray.hpp>


class RvkFramebuffer
{
public:
    void Create(FxSizedArray<VkImageView> &image_views, const RvkRenderPass& render_pass, const RvkGraphicsPipeline &pipeline, Vec2u size);
    void Create(FxSizedArray<VkImageView> &image_views, RvkGraphicsPipeline &pipeline, Vec2u size);
    void Destroy();

public:
    VkFramebuffer Framebuffer = nullptr;
    RvkGpuDevice *mDevice = nullptr;
};
