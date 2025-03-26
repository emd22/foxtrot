#pragma once

#include <ios>
#include <iostream>

#include <Core/Types.hpp>

#include "Renderer/Backend/Vulkan/ShaderList.hpp"

namespace vulkan {

class Shader
{
public:

    Shader() = default;

    Shader(const char *path, ShaderType type)
        : Type(type)
    {
        this->Load(path, type);
    }

    void Load(const char *path, ShaderType type);
    void Destroy();

    ~Shader()
    {
        this->Destroy();
    }

private:
    void CreateShaderModule(std::ios::pos_type file_size, uint32 *shader_data);

public:
    VkShaderModule ShaderModule = nullptr;
    ShaderType Type = ShaderType::Unknown;
};

}; // namespace vulkan
