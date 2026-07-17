# Architecture

This repository keeps all engine and game source visible in one solution, but uses static-library and API boundaries to make dependency direction explicit.

## Project dependency direction

```text
DirectX12OpenCampusGame.exe (Win32 launcher / composition root)
    |-- OpenCampusGame.lib
    `-- EngineFramework.lib

AnimationEventEditor.exe
    `-- EngineFramework.lib
```

- `src/Game` contains Open Campus-specific rules, scenes, actions, content IDs, and asset paths.
- `src/Framework` contains reusable engine code, public service contracts, and backend adapters.
- `src/Launcher` is the only place that creates and connects Win32, DX12, RmlUi, Effekseer, audio, and the game module.
- `tools/AnimationEventEditor` reuses `EngineFramework.lib`; it does not compile a private copy of the engine.

No DLL or opaque binary distribution is used. Static libraries are compilation boundaries only; every implementation remains in the solution.

## Important contracts

- `IRenderer`: frame drawing without command-list or descriptor-heap access.
- `IRenderDevice`: engine resource creation without exposing `ID3D12Device` to Game.
- `IAudioService`: cue registration/playback; Game owns semantic cue IDs and paths.
- `IEffectService`: effect registration/playback without exposing Effekseer.
- `IUiService` / `IUiDocument`: UI documents without exposing RmlUi.
- `Input`: platform-neutral frame snapshot populated by `Win32InputBackend`.

## Rules

1. Framework never includes Game.
2. Game never names Win32, DX12, RmlUi, Effekseer, or Assimp types.
3. Game-specific content paths do not live in Framework.
4. Vendor-native access is allowed in adapters and the Launcher composition root.
5. New physical key bindings are added to `GameActions`, not scattered through gameplay classes.

`tools/check_architecture.ps1` checks these rules and runs before EngineFramework builds.
