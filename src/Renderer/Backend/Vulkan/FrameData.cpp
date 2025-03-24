#include "FrameData.hpp"

namespace vulkan {

void FrameData::Create(GPUDevice *device)
{
    this->ImageAvailable.Create(device);
    this->RenderFinished.Create(device);

    this->InFlight.Create(device);
}

void FrameData::Destroy()
{
    this->ImageAvailable.Destroy();
    this->RenderFinished.Destroy();

    this->InFlight.Destroy();

};

} // namespace vulkan
