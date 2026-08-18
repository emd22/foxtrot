#pragma once

#include "SysThread.hpp"
#include "ThreadID.hpp"

#include <Core/SizedArray.hpp>


namespace fx {


class ThreadManager
{
public:
	ThreadManager();


	~ThreadManager();

private:
	SizedArray<SysThread> mThreads;
};


} // namespace fx
