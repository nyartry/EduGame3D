# EduGame3D

授業用3Dゲームフレームワーク。日本国内の教育機関で、3Dゲームのプログラミングを学ぶためのプロジェクトです。

A C++ / DirectX 12 framework for classroom game programming on Windows, built with Visual Studio 2022.

## Purpose and handoff

EduGame3D is intended for teaching, reading and modifying source code, and building students' own 3D games. It is designed for use across educational institutions in Japan. The framework supports different game genres without prescribing gameplay rules; the included game demonstrates the framework's APIs.

現在は公開準備中です。権利者への授業目的公衆送信補償金の分配を目指し、権利関係・利用条件・申請先を整理しています。SARTRASへの登録や分配の対象になることは未確認です。公開前の作業は [SARTRAS準備調査](docs/sartras-readiness-audit.md) と [権利台帳](docs/rights-inventory.md) を参照してください。

See [学生配布用3Dゲームひな型のコア調査・引き継ぎ](docs/student-template-core-handoff.md) for the project intent, current implementation, proposed improvements, validation results, and instructions for continuing work on another PC.

## What is included

- Win32 window creation
- Direct3D 12 device, swap chain, command queue, command list
- Root signature and graphics pipeline state
- A perspective camera, depth buffer, and simple 3D ground mesh
- Visual Studio 2022 solution and project files

## Source layout

- `src/Game`: sample gameplay, scenes, actions, and content definitions.
- `src/Framework`: reusable engine APIs, runtime systems, and visible backend adapters.
- `src/Launcher`: Win32 composition root that connects Game to concrete engine implementations.
- `tools/AnimationEventEditor`: editor executable that reuses the same static engine library.

The solution builds `EngineFramework.lib` and `GameModule.lib` as source-visible static libraries. Third-party runtime dependencies include Assimp DLLs. See [docs/architecture.md](docs/architecture.md) for dependency rules and extension points.

## Requirements

- Windows 10 or later
- Visual Studio 2022 with the Desktop development with C++ workload
- Windows 10/11 SDK
## Build

Open `EduGame3D.sln` in Visual Studio 2022, then build and run `GameApp` with `Debug|x64`.

To run the built game outside Visual Studio, use the repository root as the working directory: `.\x64\Debug\GameApp.exe`.

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
