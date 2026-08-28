# Architecture

This repository keeps all engine and game source visible in one solution, but uses static-library and API boundaries to make dependency direction explicit.

## Project dependency direction

```text
GameApp.exe (Win32 application / composition root)
    |-- GameModule.lib
    `-- EngineFramework.lib

AnimationEventEditor.exe
    `-- EngineFramework.lib
```

- `src/Game` contains Open Campus-specific rules, scenes, actions, content IDs, and asset paths.
- `src/Framework` contains reusable engine code, public service contracts, and backend adapters.
- `src/Launcher` is the only place that creates and connects Win32, DX12, RmlUi, Effekseer, audio, and the game module.
- `tools/AnimationEventEditor` reuses `EngineFramework.lib`; it does not compile a private copy of the engine.

No DLL or opaque binary distribution is used. Static libraries are compilation boundaries only; every implementation remains in the solution. Visual Studio rebuilds a `.lib` only when its project inputs changed, then relinks dependent executables as needed.

Pimpl is used selectively for backend-heavy resource and service classes. It prevents platform/vendor headers from spreading through public includes; it does not hide source code. The corresponding `Impl` definitions remain in the repository and can be read, changed, and debugged normally.

## Important contracts

- `IRenderer`: frame drawing without command-list or descriptor-heap access.
- `IRenderDevice`: engine resource creation without exposing `ID3D12Device` to Game.
- `IRenderResourceLifetime`: GPU-safe deferred destruction without exposing fence values or frame-count assumptions.
- `IAudioService`: cue registration/playback; Game owns semantic cue IDs and paths.
- `IEffectCatalog` / `IEffectPlayer`: content registration and gameplay playback without exposing Effekseer or its render lifecycle.
- `IUiService` / `IUiDocument`: UI documents, semantic click events, and state classes without exposing RmlUi or requiring Game-side hit testing.
- `Input`: platform-neutral frame snapshot; platform adapters write through the narrow `InputWriter` boundary.
- `AssetPathResolver`: one path-location policy shared by texture, model, audio, and effect adapters.

Scene loading has two explicit phases: `Prepare()` is worker-thread, CPU-only work, and `Activate()` is main-thread GPU/UI work. Scene factories receive their required services through constructors instead of a general service bag.

## Rules

1. Framework never includes Game.
2. Game never names Win32, DX12, RmlUi, Effekseer, or Assimp types.
3. Game-specific content paths do not live in Framework.
4. Vendor-native access is allowed in adapters and the Launcher composition root.
5. New physical key bindings are added to `GameActions`, not scattered through gameplay classes.
6. `DirectXMath` value types are currently an intentional shared math vocabulary. This is the remaining vendor-level public dependency; replacing it should be a deliberate math-API migration, not a set of aliases.

`tools/check_architecture.ps1` checks these rules and runs before EngineFramework builds. It also follows Framework headers transitively reachable from Game, so a backend header leaking through an otherwise innocent public include is rejected.
