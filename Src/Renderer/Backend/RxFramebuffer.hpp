#pragma once

#include "Math/FxVec2.hpp"
#include "RxPipeline.hpp"

#include <vulkan/vulkan.h>

#include <Core/FxSizedArray.hpp>


class RxFramebuffer
{
public:
    void Create(const FxSizedArray<VkImageView>& image_views, const RxRenderPass& render_pass, FxVec2u size);
    void Destroy();

    ~RxFramebuffer() { Destroy(); }

public:
    VkFramebuffer Framebuffer = nullptr;
    RxGpuDevice* mDevice = nullptr;
};
