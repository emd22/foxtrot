
#include <exception>
#include <iostream>

#define SDL_DISABLE_OLD_NAMES
#include <SDL3/SDL.h>
#include <SDL3/SDL_revision.h>

#include <SDL3/SDL_main.h>

#include <Core/Types.hpp>

#include "Backend/Vulkan.hpp"

static bool Running = true;

template <typename T, typename... Types>
void Panic(const char *fmt, T first, Types... items) {
    printf(fmt, first, items...);
    puts("");

    std::terminate();
}

template <typename... Types>
void Panic(const char *fmt, VkResult result, Types... items) {
    printf(fmt, items...);
    puts("");

    printf("=> Vulkan Err: %d\n", result);

    std::terminate();
}

class Window {
public:
    Window(const std::string &title, int32 width, int32 height) {
        const uint64 window_flags = SDL_WINDOW_VULKAN;

        mWindow = SDL_CreateWindow(title.c_str(), width, height, window_flags);

        if (mWindow == nullptr) {
            Panic("Could not create SDL window (SDL err: %s)", SDL_GetError());
        }
    }

    SDL_Window *GetWindow() { return this->mWindow; }

    ~Window() {
        SDL_DestroyWindow(mWindow);
    }

private:
    SDL_Window *mWindow;
};


inline void ProcessEvents() {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
                Running = false;
                break;
            default:;
        }
    }
}

int main() {

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        Panic("Could not initialize SDL! (SDL err: %s)\n", SDL_GetError());
    }

    Window window("Test window", 1024, 720);

    VkRenderBackend backend;
    backend.SelectWindow(window.GetWindow());
    backend.Init();

    while (Running) {
        ProcessEvents();
    }

    return 0;
}
