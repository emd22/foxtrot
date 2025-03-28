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

// #include <Renderer/FxShader.hpp>

// class FxVulkanShader final : public FxShader
// {
// public:
//     FxVulkanShader() = default;
//     FxVulkanShader(FxShader::Type type, std::string &path)
//     {
//         this->Load(type, path);
//     }

//     void Load(FxShader::Type, std::string &path) override;
//     void Destroy() override;

//     Type GetType() { return this->mType; }

//     ~FxVulkanShader()
//     {
//         this->Destroy();
//     }

// };

}; // namespace vulkan
