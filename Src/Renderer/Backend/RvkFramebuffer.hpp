#pragma once

#include <vulkan/vulkan.h>

#include "Math/Vec2.hpp"
#include "RvkPipeline.hpp"

#include <Core/FxSizedArray.hpp>


class RvkFramebuffer
{
public:
    void Create(const FxSizedArray<VkImageView> &image_views, const RvkGraphicsPipeline &pipeline, FxVec2u size);
    void Destroy();

    ~RvkFramebuffer()
    {
        Destroy();
    }

public:
    VkFramebuffer Framebuffer = nullptr;
    RvkGpuDevice *mDevice = nullptr;
};
