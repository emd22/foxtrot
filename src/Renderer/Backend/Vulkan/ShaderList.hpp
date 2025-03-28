#pragma once

#include "Renderer/Backend/RenderPanic.hpp"
#include "vulkan/vulkan_core.h"
#include <Core/Types.hpp>
#include <Core/StaticArray.hpp>

#include <Core/Log.hpp>
#include <vulkan/vulkan.h>

enum class ShaderType
{
    Unknown,
    Vertex,
    Fragment,
};

class ShaderInfo
{
public:
    ShaderInfo() = default;

    ShaderInfo(ShaderType type, VkShaderModule module)
        : ShaderModule(module), mShaderType(type)
    {
    }



    VkShaderStageFlagBits GetStageBit()
    {
        switch (this->mShaderType) {
            case ShaderType::Unknown:
                Panic_("ShaderList", "Attempting to get shader stage bit of ShaderType::Unknown!", 0);
            case ShaderType::Vertex:
                return VK_SHADER_STAGE_VERTEX_BIT;
            case ShaderType::Fragment:
                return VK_SHADER_STAGE_FRAGMENT_BIT;
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

public:
    VkShaderModule Vertex = nullptr;
    VkShaderModule Fragment = nullptr;
};
