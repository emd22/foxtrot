#include "AssetTicket.hpp"

#include <Core/Thread/ThreadManager.hpp>

namespace fx {

static uint64 sCurrentTicketDataID = 0;

AssetTicketData::AssetTicketData()
{
	Assert(gThreadManager->IsMainThread() == true);
	ID = (sCurrentTicketDataID++);
}

void AssetTicketData::MarkAndSignalLoaded()
{
	if (bIsLoaded.load()) {
		return;
	}

	IsFinishedNotifier.Signal();

	bIsUploadedToGpu = true;
	bIsUploadedToGpu.notify_all();

	bIsLoaded.store(true);
}

void AssetTicketData::OnLoaded(void* item, const OnLoadFunc& on_loaded_callback)
{
	std::lock_guard guard(mCallbackMutex);

	// If the asset has already been loaded, call the callback immediately.
	if (IsFinishedNotifier.IsSignalled()) {
		on_loaded_callback(item);
		return;
	}

	mOnLoadedCallbacks.push_back(on_loaded_callback);
}

void AssetTicketData::OnError(const OnErrorFunc& on_error_callback)
{
	std::lock_guard guard(mCallbackMutex);

	// If the asset has already been loaded, call the callback immediately.
	if (IsFinishedNotifier.IsSignalled()) {
		on_error_callback();
		return;
	}

	mOnErrorCallback = on_error_callback;
}


void AssetTicketData::SignalUploadedToGpu()
{
	bIsUploadedToGpu = true;
	bIsUploadedToGpu.notify_all();
}


void AssetTicketData::SignalFinished() { IsFinishedNotifier.Signal(); }


} // namespace fx
