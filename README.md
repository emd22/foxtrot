# Foxtrot Engine

## What is Foxtrot?
Foxtrot is a modern Vulkan game engine written in C++20.
## Support
Since Foxtrot is a very new engine, everything is tailored pretty heavily to my personal hardware. Since most of my development is done on macOS, this means that currently there is no Windows or Linux support, but will be added soon.

### MoltenVK
For macOS support, MoltenVK is required.

At the time of writing the LunarG bindings ship with an out of date version of MoltenVK as well as Vulkan headers. This means that in order to be able to use Vulkan 1.3 you will need to copy the MoltenVK dynamic library and headers to the install location of the SDK(most likely `~/VulkanSDK/1.4.313.1/macOS/.../`).


## Dependencies
Currently, Foxtrot only relies on
- SDL3
- TurboJPEG
- Vulkan
- MoltenVK _(optional)_

- As well as header-only libraries cgltf and VMA.

## Building
To build Foxtrot, you will need to have CMake installed.

- Create a build folder and navigate to it.
- Generate the project build scripts using CMake (`cmake ../`)
- Build the project using `make` or `ninja`.
