#pragma once

#define VMA_DEBUG_LOG(...) Log::Warning(__VA_ARGS__)

#include <ThirdParty/vk_mem_alloc.h>

#include <Renderer/FxDeletionObject.hpp>

void Fx_Fwd_AddToDeletionQueue(const FxDeletionObject::FuncType& func);
void Fx_Fwd_AddGpuBufferToDeletionQueue(const VkBuffer& buffer, const VmaAllocation& allocation);
