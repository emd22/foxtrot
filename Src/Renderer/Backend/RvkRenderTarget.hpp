#pragma once

#include <vector>

#include <Math/Vec2.hpp>

#include "RvkFramebuffer.hpp"
#include "RvkRenderPass.hpp"
#include "RvkTexture.hpp"
#include "RvkDescriptors.hpp"

// XXX: TEMP!!!
class RvkDeferredRenderTarget
{
public:
    void Create(const Vec2u& size, RvkGraphicsPipeline* pipeline);
    void SetupDescriptors();

    void Destroy();

public:
    RvkImage Positions;
    RvkImage Normals;
    RvkImage Albedo;

    RvkImage Depth;

    RvkRenderPass RenderPass;
    RvkFramebuffer Framebuffer;

    Vec2u Size = Vec2u::Zero;

private:
    RvkDescriptorSet mDescriptorSet;
    RvkGraphicsPipeline* mPipeline;
};


class RvkRenderTarget
{
public:
    RvkRenderTarget() = default;

    void Create(const Vec2u& size, const std::vector<RvkImage>& attachments);


public:
    std::vector<RvkImage> Attachments;

    RvkRenderPass RenderPass;
    RvkFramebuffer Framebuffer;

    Vec2u Size = Vec2u::Zero;
};
