#pragma once

#include "vulkan/vulkan_core.h"
#include <Core/Types.hpp>
#include <Core/StaticArray.hpp>

#include <vulkan/vulkan.h>

enum class ShaderType
{
    Vertex,
    Fragment,
};

class ShaderInfo
{
public:
    ShaderInfo() = default;

    ShaderInfo(ShaderType type, VkShaderModule module)
        : mShaderType(type), ShaderModule(module)
    {
    }

    VkShaderStageFlagBits GetStageBit()
    {
        switch (this->mShaderType) {
            case ShaderType::Vertex:
                return VK_SHADER_STAGE_VERTEX_BIT;
            case ShaderType::Fragment:
                return VK_SHADER_STAGE_VERTEX_BIT;
        }
    }

public:
    VkShaderModule ShaderModule = nullptr;

private:
    ShaderType mShaderType;
};

class ShaderList
{
public:
    StaticArray<ShaderInfo> GetShaderStages()
    {
        StaticArray<ShaderInfo> shader_stages(2);

        if (this->Vertex != nullptr) {
            shader_stages.Insert(ShaderInfo(ShaderType::Vertex, this->Vertex));
        }
        if (this->Fragment != nullptr) {
            shader_stages.Insert(ShaderInfo(ShaderType::Fragment, this->Fragment));
        }

        return shader_stages;
    }

    VkShaderModule Vertex = nullptr;
    VkShaderModule Fragment = nullptr;
};
