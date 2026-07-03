# Animation Event Editor

FBX animation event editor for this DirectX12 Open Campus project.

## Build

Open `DirectX12OpenCampusGame.sln` in Visual Studio.

The solution contains two projects:

- `DirectX12OpenCampusGame`
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
