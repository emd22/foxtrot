#include "FxAssetManager.hpp"
#include "Core/FxPanic.hpp"
#include "Core/FxSizedArray.hpp"

#include "FxModel.hpp"
#include "FxImage.hpp"

#include "Loader/FxGltfLoader.hpp"
#include "Loader/FxJpegLoader.hpp"


#include <atomic>
#include <chrono>
#include <thread>

#include <Core/Types.hpp>

#include <Core/Defines.hpp>


////////////////////////////////////
// Asset Worker
////////////////////////////////////

void FxAssetWorker::Create()
{
    while (Running.test_and_set(std::memory_order_acquire)) {
        Running.wait(true, std::memory_order_relaxed);
    }

    Thread = std::thread([this]() { this->Update(); });

}

void FxAssetWorker::Update()
{
    FxAssetManager& manager = FxAssetManager::GetInstance();

    while (Running.test()) {
        ItemReady.WaitForData();
        ItemReady.Reset();

        if (!Running.test()) {
            break;
        }

        if (Item.RawData != nullptr && Item.DataSize > 0) {
            LoadStatus = Item.Loader->LoadFromMemory(Item.Asset, Item.RawData, Item.DataSize);
        }
        else {
            // Call our specialized loader to load the asset file
            LoadStatus = Item.Loader->LoadFromFile(Item.Asset, Item.Path);
        }


        // Mark that we are waiting for the data to be uploaded to the GPU
        DataPendingUpload.test_and_set();

        // Signal the main asset thread that we are done
        manager.DataLoaded.SignalDataWritten();
    }
}


////////////////////////////////////
// Asset Manager
////////////////////////////////////


void FxAssetManager::Start(int32 thread_count)
{
    mThreadCount = thread_count;
    mActive.test_and_set();

    // Allocate the workers
    mWorkerThreads.InitCapacity(thread_count);

    for (int32 i = 0; i < thread_count; i++) {
        // 'Insert' a new worker and get its pointer
        FxAssetWorker* worker = mWorkerThreads.Insert();

        // Create the worker from the newly inserted pointer
        worker->Create();
    }

    std::thread* thread = new std::thread([this]() { FxAssetManager::AssetManagerUpdate(); });
    mAssetManagerThread = thread;
}

void FxAssetManager::Shutdown()
{
    if (!mActive.test()) {
        return;
    }

    mActive.clear();
    ItemsEnqueuedNotifier.Kill();

    for (auto& worker : mWorkerThreads) {
        worker.Running.clear();
        worker.Running.notify_one();

        // The worker waits on the ItemReady notifier, so we kill the notifier
        worker.ItemReady.Kill();

        worker.Thread.join();
    }


    mAssetManagerThread->join();
    delete mAssetManagerThread;

    // Free the workers thread array here---
    // this calls the destructors for each worker thread,
    // which frees the `FxRef`'s to the data
    mWorkerThreads.Free();

    // mLoadQueue.Destroy();
}

template<>
void FxAssetManager::LoadAsset<FxModel>(FxRef<FxModel> asset, const std::string& path)
{
    DoLoadAsset<FxModel, FxGltfLoader, FxAssetType::Model>(asset, path);
}

template<>
void FxAssetManager::LoadAsset<FxImage>(FxRef<FxImage> asset, const std::string& path)
{
    DoLoadAsset<FxImage, FxJpegLoader, FxAssetType::Image>(asset, path);
}


template<>
void FxAssetManager::LoadFromMemory<FxImage>(FxRef<FxImage> asset, const uint8* data, uint32 data_size)
{
    DoLoadFromMemory<FxImage, FxJpegLoader, FxAssetType::Image>(asset, data, data_size);
}


void FxAssetManager::CheckForUploadableData()
{
    for (auto& worker : mWorkerThreads) {
        // If there are no uploads pending, skip the worker
        if (!worker.DataPendingUpload.test()) {
            continue;
        }

        auto& loaded_item = worker.Item;

        // The asset was successfully loaded, upload to GPU
        if (worker.LoadStatus == FxBaseLoader::Status::Success) {
            // Load the resouce into GPU memory
            loaded_item.Loader->CreateGpuResource(loaded_item.Asset);
            while (!loaded_item.Asset->IsUploadedToGpu) {
                loaded_item.Asset->IsUploadedToGpu.wait(true);
            }

            loaded_item.Asset->IsFinishedNotifier.SignalDataWritten();
            loaded_item.Asset->mIsLoaded.store(true);

            // Call the OnLoaded callback if it was registered
            if (!loaded_item.Asset->mOnLoadedCallbacks.empty()) {
                for (auto& callback : loaded_item.Asset->mOnLoadedCallbacks) {
                    callback(loaded_item.Asset);
                }
                // loaded_item.Asset->mOnLoadedCallback(loaded_item.Asset);
            }
        }
        else if (worker.LoadStatus == FxBaseLoader::Status::Error) {
            loaded_item.Asset->IsFinishedNotifier.SignalDataWritten();

            // There was an error, call the OnError callback if it was registered
            if (loaded_item.Asset->mOnErrorCallback) {
                loaded_item.Asset->mOnErrorCallback(loaded_item.Asset);
            }
        }
        else if (worker.LoadStatus == FxBaseLoader::Status::None) {
            loaded_item.Asset->IsFinishedNotifier.SignalDataWritten();

            FxPanic("FxAssetManager", "Worker status is none!", 0);
        }

        ItemsEnqueued.clear();
        worker.IsBusy.clear();
    }
}

bool FxAssetManager::CheckWorkersBusy()
{
    for (auto& worker : mWorkerThreads) {
        if (worker.IsBusy.test()) {
            return true;
        }
    }
    return false;
}

void FxAssetManager::CheckForItemsToLoad()
{
    FxAssetQueueItem item;
    if (!mLoadQueue.PopIfAvailable(&item)) {
        // The load queue is currently in use(uploaded to), skip for now.
        return;
    }

    FxAssetWorker* worker = FindWorkerThread();

    // No workers available, poll until one becomes available
    while (worker == nullptr) {
        Log::Debug("No workers available; Polling for worker thread...");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Check to see if any threads opened up
        worker = FindWorkerThread();
    }

    // Submit the item we want to load
    worker->SubmitItemToLoad(item);
}

void FxAssetManager::AssetManagerUpdate()
{
    while (mActive.test()) {
        bool is_busy = CheckWorkersBusy();

        // If any of the workers are still marked as busy, there is data
        // either to be uploaded or is currently being loaded.
        if (is_busy) {
            // Loop while we are busy to check for when we are pending upload, as well
            // as check for new arrivals.
            while (CheckWorkersBusy()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(80));

                CheckForUploadableData();
                CheckForItemsToLoad();
            }
        }

        // There are no busy workers remaining, wait for the next item to be enqueued.
        ItemsEnqueuedNotifier.WaitForData();
        ItemsEnqueuedNotifier.Reset();
        if (!mActive.test()) {
            break;
        }

        CheckForItemsToLoad();
    }
}

FxAssetWorker* FxAssetManager::FindWorkerThread()
{
    for (FxAssetWorker& worker : mWorkerThreads) {
        if (!worker.IsBusy.test()) {
            return &worker;
        }
    }
    return nullptr;
}

FxAssetManager& FxAssetManager::GetInstance()
{
    static FxAssetManager AssetManager;

    return AssetManager;
}
