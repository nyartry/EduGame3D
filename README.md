# Game Framework

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

The solution builds `EngineFramework.lib` and `GameModule.lib` as source-visible compilation boundaries. No DLL is used. See [docs/architecture.md](docs/architecture.md) for dependency rules and extension points.

## Requirements

- Windows 10 or later
- Visual Studio 2022 with the Desktop development with C++ workload
- Windows 10/11 SDK
## Build

Open `GameFramework.sln` in Visual Studio 2022, then build and run `GameApp` with `Debug|x64`.

## Camera controls

- `1`: existing interactive follow camera (default)
- `2`: critically damped spring follow camera
- `3`: first-person camera
- `4`: orbit camera
- `5`: Catmull-Rom spline camera; press `5` again to restart the shot
- Arrow keys control the active interactive camera. Hold `Shift` with Up/Down to zoom where supported, and hold `Z` to align the view with the player.

## Dependency check

`tools/check_architecture.ps1` prevents Game from directly or transitively depending on Win32, DX12, RmlUi, Effekseer, Assimp, or other backend implementation types. It runs automatically when `EngineFramework` builds.

See [error handling and TDD](docs/error-handling.md) for failure recovery, diagnostic ownership, and regression test commands.

The solution also includes `VisualStudioTests`, a native C++ test project. Build it and open **Test > Test Explorer** to run or debug individual tests. See [Visual Studio testing](docs/visual-studio-tests.md) for setup and command-line equivalents.
