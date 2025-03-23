#include "RenderBackend.hpp"
#include "vulkan/vulkan_core.h"

#include <vulkan/vulkan.hpp>

struct SDL_Window;

class VkRenderBackend final : RenderBackend {
public:
    using ExtensionList = std::vector<VkExtensionProperties>;
    using ExtensionNames = std::vector<const char *>;

    void Init();
    ~VkRenderBackend();

    void SelectWindow(SDL_Window *window)
    {
        this->mWindow = window;
    }

private:
    void InitVulkan();
    void CreateSurfaceFromWindow();

    ExtensionList QueryInstanceExtensions(bool invalidate_previous = false);
    ExtensionNames MakeInstanceExtensionList(ExtensionNames user_requested_extensions);
    ExtensionNames CheckExtensionsAvailable(ExtensionNames requested_extensions);

    std::vector<VkLayerProperties> GetAvailableValidationLayers();

private:
    bool mInitialized = false;

    VkInstance mInstance = nullptr;
    VkSurfaceKHR mWindowSurface = nullptr;

    SDL_Window *mWindow = nullptr;

    VkDebugUtilsMessengerEXT mDebugMessenger;

    ExtensionList mAvailableExtensions;
};
