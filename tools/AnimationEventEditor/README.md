# Animation Event Editor

FBX animation event editor for this DirectX12 Open Campus project.

## Build

Open `GameFramework.sln` in Visual Studio.

The solution contains four projects:

- `GameApp`
- `GameModule`
- `EngineFramework`
- `AnimationEventEditor`

Build `AnimationEventEditor` with `Debug|x64` or `Release|x64`.

## Run

After building, run one of these files:

- `tools/AnimationEventEditor/bin/Debug/AnimationEventEditor.exe`
- `tools/AnimationEventEditor/bin/Release/AnimationEventEditor.exe`

The original Visual Studio output is also here:

- `x64/Debug/AnimationEventEditor.exe`
- `x64/Release/AnimationEventEditor.exe`

## Basic Workflow

1. Open `AnimationEventEditor.exe`.
2. Choose `File > Open FBX...`.
3. Select an animation in the `Asset` panel.
4. Press `Play` or scrub the timeline.
5. Click the timeline or press `Add Event At Current Time`.
6. Edit the event type, bone, and cue in `Event Properties`.
7. Choose `File > Save Events`.

The editor saves `.anim_events.json` files.

When an FBX is opened, the editor also looks for a sibling `.anim_events.json` file and loads it automatically.

When opening or saving files in the editor, FBX and event JSON file names must use printable ASCII characters: letters, digits, spaces, and ASCII symbols valid in Windows file names. For example, use `walk.fbx` and `walk.anim_events.json`. Folder names may contain Japanese or other Unicode characters.

If a file name is rejected, the Status panel explains the restriction. The current document, edit history, and saved files are preserved.

## Editing

- `Edit > Undo` / `Ctrl+Z` reverts event edits.
- `Edit > Redo` / `Ctrl+Y` reapplies reverted event edits.
- `File > Open Events...` loads an existing `.anim_events.json` for the current model.
- `View > Reset Layout` restores the default panel arrangement and rewrites the saved ImGui layout.
