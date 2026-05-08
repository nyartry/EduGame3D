# DirectX12 Open Campus Game

Windows desktop DirectX 12 starter project for Visual Studio 2022.

## What is included

- Win32 window creation
- Direct3D 12 device, swap chain, command queue, command list
- Root signature and graphics pipeline state
- A perspective camera, depth buffer, and simple 3D ground mesh
- Visual Studio 2022 solution and project files

## Source layout

- `src/main.cpp`: Application entry point and top-level exception handling.
- `src/Win32Application.*`: Window creation and the Win32 message loop.
- `src/Game.*`: Game lifecycle bridge for update/render ticks.
- `src/Dx12Renderer.*`: Direct3D 12 device setup, GPU resources, and drawing.
- `src/Ground.*`: Ground field object and its vertex buffer.
- `src/Vertex.h`: Shared vertex format.
- `src/Common.h`: Small shared helpers.

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
