#pragma once

#include <Core/Defines.hpp>

#include <Core/Log.hpp>
#include <Renderer/Backend/Vulkan/Util.hpp>

#include <vulkan/vulkan.h>

template <typename T, typename... Types>
void FxPanic_(const char * const module, const char *fmt, T first, Types... items)
{
    Log::LogSeverityText<Log::Severity::Fatal>();

    if (module != nullptr) {
        Log::Write("%s: ", module);
    }

    Log::Write(fmt, items...);
    Log::Write("\n");

    std::terminate();
}

template <typename... Types>
void FxPanic_(const char *module, const char *fmt, VkResult result, Types... items)
{
    Log::LogSeverityText<Log::Severity::Fatal>();

    if (module != nullptr) {
        Log::Write("%s: ", module);
    }

    Log::Write(fmt, items...);
    Log::Write("\n");

    printf("=> Vulkan Err: %s\n", VulkanUtil::ResultToStr(result));

    std::terminate();
}

#define FxPanic(...) \
    FxPanic_(ArModuleName__, __VA_ARGS__)
