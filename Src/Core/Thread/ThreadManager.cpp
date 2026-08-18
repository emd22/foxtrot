
#include "ThreadManager.hpp"

#include "ThreadID.hpp"

#include <Renderer/Limits.hpp>

namespace fx {

const ThreadID ThreadID::scMainThread = ThreadID(0);

ThreadManager::ThreadManager() { mThreads.InitCapacity(Limits::MaxConcurrentThreads); }

ThreadManager::~ThreadManager() {}


} // namespace fx
