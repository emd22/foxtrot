#pragma once

#include <atomic>

#include <Core/FxDataNotifier.hpp>

#include <Core/FxRef.hpp>

#include <functional>

/**
 * An asset base class for other assets to be derived from.
 *
 * @see FxModel
 */
class FxAssetBase
{
protected:
    FxAssetBase()
    {
    }


    // template <typename T>
    // friend class FxRef;

public:

    // using OnLoadFunc = void (*)(FxRef<FxBaseAsset> asset);
    using OnLoadFunc = std::function<void (FxRef<FxAssetBase>)>;
    using OnErrorFunc = void (*)(FxRef<FxAssetBase> asset);

    virtual void WaitUntilLoaded()
    {
         if (IsFinishedNotifier.IsDone()) {
             return;
         }

        IsFinishedNotifier.WaitForData(true);
    }

    /** Returns true if the asset has been loaded and is in GPU memory. */
    inline bool IsLoaded() const
    {
        if (mIsLoaded) {
            printf("is loaded\n");
        }
        return mIsLoaded.load();
    }


    virtual void Destroy() = 0;

    virtual ~FxAssetBase()
    {
    }

    /**
     * Attaches a new callback for when the Asset is finished being loaded by the AssetManager.
     */
    // void OnLoaded(CallbackFunc &&on_loaded) { AttachCallback(FxAssetCallbackType::OnLoaded, std::move(on_loaded)); }



    void OnLoaded(const OnLoadFunc& on_loaded_callback)
    {
        // If the asset has already been loaded, call the callback immediately.
        if (IsFinishedNotifier.IsDone()) {
            on_loaded_callback(this);
            return;
        }
        
        printf("Registered onload callback\n");

        mOnLoadedCallbacks.push_back(on_loaded_callback);
    }

    void OnError(const OnErrorFunc& on_error_callback)
    {
        if (IsFinishedNotifier.IsDone()) {
            // on_error_callback(this);
            return;
        }

        mOnErrorCallback = on_error_callback;
    }

public:
    FxDataNotifier IsFinishedNotifier;
    std::atomic_bool IsUploadedToGpu = false;

protected:
    std::vector<OnLoadFunc> mOnLoadedCallbacks;
    // OnLoadFunc mOnLoadedCallback = nullptr;
    OnErrorFunc mOnErrorCallback = nullptr;

    std::atomic_bool mIsLoaded = false;

    friend class FxAssetManager;
};
