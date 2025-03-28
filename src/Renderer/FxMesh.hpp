#pragma once

#include "Renderer/Backend/Vulkan/FrameData.hpp"
#include "Renderer/Renderer.hpp"
#include "vulkan/vulkan_core.h"
#include <Renderer/Backend/Vulkan/GPUBuffer.hpp>

// TEMP
using namespace vulkan;

class FxMesh
{
public:
    FxMesh() = default;

    FxMesh(StaticArray<Vertex> &vertices)
    {
        this->mVertexBuffer.Create(FxGPUBuffer<Vertex>::UsageType::Vertices, vertices.Size);
        this->mVertexBuffer.Upload(vertices);
    }

    FxMesh(StaticArray<Vertex> &vertices, StaticArray<uint32> &indices)
        : FxMesh(vertices)
    {
        this->mIndexBuffer.Create(FxGPUBuffer<uint32>::UsageType::Indices, indices.Size);
        this->mIndexBuffer.Upload(indices);
    }

    void Render()
    {
        const VkDeviceSize offset = 0;
        const FrameData *frame = RendererVulkan->GetFrame();

        vkCmdBindVertexBuffers(frame->CommandBuffer.CommandBuffer, 0, 1, &this->mVertexBuffer.Buffer, &offset);
        vkCmdDraw(frame->CommandBuffer.CommandBuffer, this->mVertexBuffer.Size, 1, 0, 0);
    }


private:
    FxGPUBuffer<Vertex> mVertexBuffer;
    FxGPUBuffer<uint32> mIndexBuffer;
};
