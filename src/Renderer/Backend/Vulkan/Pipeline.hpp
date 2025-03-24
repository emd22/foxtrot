#pragma once

#include "ShaderList.hpp"
#include "RenderPass.hpp"

#include <Core/StaticArray.hpp>

#include <vulkan/vulkan.h>

namespace vulkan {

struct Vertex
{
    float32 Position[3];
    float32 Normal[3];
};

struct VertexInfo
{
    VkVertexInputBindingDescription binding;
    StaticArray<VkVertexInputAttributeDescription> attributes;
};


class GraphicsPipeline
{
public:
    void Create(ShaderList shader_list);
    void Destroy();

    void Bind(VkCommandBuffer command_buffer);

private:
    VertexInfo MakeVertexInfo();
    void CreateLayout();

public:
    VkPipelineLayout Layout = nullptr;
    VkPipeline Pipeline = nullptr;

    vulkan::RenderPass RenderPass;
private:
    GPUDevice *mDevice = nullptr;

    ShaderList mShaders;

};

}; // namespace vulkan
