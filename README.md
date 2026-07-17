# DirectX12 Open Campus Game

Windows desktop DirectX 12 starter project for Visual Studio 2022.

## What is included

- Win32 window creation
- Direct3D 12 device, swap chain, command queue, command list
- Root signature and graphics pipeline state
- A perspective camera, depth buffer, and simple 3D ground mesh
- Visual Studio 2022 solution and project files

## Source layout

- `src/Game`: Open Campus-specific gameplay, scenes, actions, and content definitions.
- `src/Framework`: reusable engine APIs, runtime systems, and visible backend adapters.
- `src/Launcher`: Win32 composition root that connects Game to concrete engine implementations.
- `tools/AnimationEventEditor`: editor executable that reuses the same static engine library.

The solution builds `EngineFramework.lib` and `OpenCampusGame.lib` as source-visible compilation boundaries. No DLL is used. See [docs/architecture.md](docs/architecture.md) for dependency rules and extension points.

## Requirements

- Windows 10 or later
- Visual Studio 2022 with the Desktop development with C++ workload
- Windows 10/11 SDK
## Build

Open `DirectX12OpenCampusGame.sln` in Visual Studio 2022, then build and run with `Debug|x64`.

## Dependency check

`tools/check_architecture.ps1` prevents Game from directly depending on Win32, DX12, RmlUi, Effekseer, or Assimp implementation types. It runs automatically when `EngineFramework` builds.
