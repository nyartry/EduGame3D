# DirectX12 Open Campus Game

Windows desktop DirectX 12 starter project for Visual Studio 2022.

## What is included

- Win32 window creation
- Direct3D 12 device, swap chain, command queue, command list
- Root signature and graphics pipeline state
- A simple animated triangle
- Visual Studio 2022 solution and project files

## Requirements

- Windows 10 or later
- Visual Studio 2022 with the Desktop development with C++ workload
- Windows 10/11 SDK
## Build

Open `DirectX12OpenCampusGame.sln` in Visual Studio 2022, then build and run with `Debug|x64`.

## Good next template upgrades

- Add DirectXTK12 for sprites, text, models, input helpers, and audio.
- Split rendering, input, scene, and asset loading into separate modules.
- Add a `Content/` directory for textures, models, and compiled shaders.
- Move shader code from `src/main.cpp` to `.hlsl` files once the first build is stable.
