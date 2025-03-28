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

    ~RendererStateInstance()
    {
        this->Destroy();
    }

    vulkan::VkRenderBackend Vulkan;
};

void SetRendererState(RendererStateInstance *instance);

void AssertRendererExists();

extern RendererStateInstance *RendererState;
