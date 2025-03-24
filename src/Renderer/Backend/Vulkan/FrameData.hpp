#pragma once

#include "Semaphore.hpp"
#include "Fence.hpp"

#include "CommandPool.hpp"
#include "CommandBuffer.hpp"

namespace vulkan {

class FrameData
{
public:
    void Create(GPUDevice *device);
    void Destroy();
public:
    CommandPool CommandPool;
    CommandBuffer CommandBuffer;

    Semaphore ImageAvailable;
    Semaphore RenderFinished;
    Fence InFlight;
};

};
