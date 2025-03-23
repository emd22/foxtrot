#pragma once

#include "Vulkan/Util.hpp"
#include <Util/Log.hpp>

#include <vulkan/vulkan.h>

template <typename T, typename... Types>
void RenderPanic(const char *module, const char *fmt, T first, Types... items)
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
void RenderPanic(const char *module, const char *fmt, VkResult result, Types... items)
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
