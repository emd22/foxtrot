#pragma once

#include "Backend/Vulkan.hpp"

struct RendererStateInstance
{
public:
    void Destroy()
    {
        if (this->Vulkan.Initialized) {
            this->Vulkan.Destroy();
        }
    }

    vulkan::VkRenderBackend Vulkan;
};

void AssertRendererExists();

extern RendererStateInstance RendererState;
