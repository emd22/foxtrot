#include "RvkShader.hpp"
#include "ShaderList.hpp"
#include "../Renderer.hpp"

#include <fstream>
#include <ios>
#include <iostream>

#include <Core/Log.hpp>
#include <Core/Types.hpp>



void RvkShader::Load(const char *path, RvkShaderType type)
{
    Type = type;

    // TODO: add to asset loading thread

    std::ifstream file;
    file.open(path, std::iostream::binary);

    if (!file.is_open()) {
        Log::Error("Could not open shader file [%s]", path);
        return;
    }

    file.seekg(0, std::ios::end);
    const auto file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    // as we are aliging to uint32* (4 bytes), we should ensure there
    // is enough memory allocated
    const auto buffer_size =  ((file_size / sizeof(uint32)) + 1) * sizeof(uint32);

    // TODO: use pooled memory!
    char *file_buffer = new char[buffer_size];

    file.read(file_buffer, file_size);
    file.close();

    CreateShaderModule(file_size, (uint32 *)file_buffer);

    delete[] file_buffer;
}

void RvkShader::Destroy()
{
    if (ShaderModule == nullptr) {
        return;
    }

    RvkGpuDevice *device = Renderer->GetDevice();
    vkDestroyShaderModule(device->Device, ShaderModule, nullptr);
}

void RvkShader::CreateShaderModule(std::ios::pos_type file_size, uint32 *shader_data)
{
    const VkShaderModuleCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = (uint32)file_size,
        .pCode = (uint32_t *)shader_data,
    };

    RvkGpuDevice *device = Renderer->GetDevice();

    const VkResult status = vkCreateShaderModule(device->Device, &create_info, nullptr, &ShaderModule);

    if (status != VK_SUCCESS) {
        Log::Error("Could not create vulkan shader module...");
        return;
    }
}
