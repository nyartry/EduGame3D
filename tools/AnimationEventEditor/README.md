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

## Tuning Combo Input

Open `Content/Models/Player/Mma Kick/Mma Kick.fbx` to edit the game's attack input window. Its sibling `Mma Kick.anim_events.json` loads automatically. The clip is `mixamo.com`, with a duration of 1.6 seconds (48 ticks at 30 ticks per second).

- Select `ComboWindowOpen` to adjust when the next attack can be requested. The initial time is 0.5 seconds.
- Select `ComboWindowClose` to adjust when that input window ends. The initial time is 1.2 seconds.
- Change `Time` in `Event Properties` or drag the event on the timeline. Keep `0 < open < close <= 1.6`. Leave `Bone` and `Cue` empty.
- Save with `File > Save Events`, then restart the game to use the updated times. No timing constants or new JSON format are needed.

The game plays each attack once through its actual clip endpoint. A fresh X press inside the window queues one next attack; early and late presses are ignored, and holding X does not repeat attacks. Closing the window keeps an already queued attack. The queued attack starts at the beginning of the next simulation update after completion, so endpoint sound and effect events can be delivered first. With the current asset set, the combo repeats the same kick.

Try a single press, an early second press, a press inside the window, a late press, and a held key after each adjustment. The supplied times are starting values for this clip.
