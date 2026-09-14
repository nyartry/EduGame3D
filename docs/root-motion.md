# Root motion configuration

`SkinnedMeshActor` and `Player` share `RootMotionSettings`. Root motion supplies
animation translation; the animation pose still plays in every mode. The current
implementation does not extract root rotation.

## Horizontal movement

| Mode | Final X/Z displacement |
| --- | --- |
| `Ignore` | Programmatic movement only |
| `Blend` | Programmatic movement × (1 − weight) + animation movement × weight |
| `Apply` | Animation movement only |

`Blend` defaults to 0.5. Its weight is clamped to 0–1 when evaluated; non-finite
weights resolve to 0. Root motion is rotated from model space to world space
before blending. Both inputs are displacements for the current update, not speeds.
For a `SkinnedMeshActor` with no programmatic movement, Blend scales the animation
displacement against zero.

Set the initial policy in `SkinnedMeshActorDefinition::rootMotion` (for players,
`PlayerDefinition::mesh.rootMotion`). `HumanoidPlayer.cpp` contains an explicit example.
Initialization loads this definition; change the policy at runtime after
`Initialize()` using the inherited API:

```cpp
player.SetRootMotionSettings({
    .mode = RootMotionMode::Blend,
    .blendWeight = 0.5f,
    .verticalMode = RootMotionVerticalMode::Ignore
});

// Select one mode as needed; changing mode does not change the vertical policy.
player.SetRootMotionMode(RootMotionMode::Ignore);
player.SetRootMotionMode(RootMotionMode::Apply);
player.SetRootMotionMode(RootMotionMode::Blend);
player.SetRootMotionBlendWeight(0.25f);
```

Apply uses animation movement even when it is zero (for example, an in-place
clip). Choose Ignore or Blend when programmatic movement must contribute.

## Jump, gravity, and optional animation Y

The settings type defaults to horizontal Apply with vertical Ignore. The included
`HumanoidPlayer` explicitly uses horizontal Ignore and vertical Ignore because
EduHuman's animations stay in place; input supplies its horizontal movement.
Jump and gravity remain programmatic at every blend
weight. Collision resolution still constrains the resulting position.

`RootMotionVerticalMode::Apply` explicitly adds weighted animation Y. It does
not scale or replace jump velocity or gravity. Once a programmatic jump is
accepted, animation Y is suppressed for its entire ascent and descent, through
landing. This protection starts on the takeoff update; root-driven motion alone
does not activate it. Disabling gravity cancels this programmatic jump state.

To let animation drive all three translation axes without gravity or a
programmatic jump, configure a player explicitly:

```cpp
player.SetRootMotionSettings({
    .mode = RootMotionMode::Apply,
    .verticalMode = RootMotionVerticalMode::Apply
});
player.SetGravityEnabled(false);
```

Floor collision remains enabled in this configuration. The vertical motion
solver tests from the position before animation movement to the final position,
so animation-driven descent also lands on platforms. Its return value indicates
an accepted programmatic jump, not simply a transition into the air.

## Regression tests

Run `powershell -NoProfile -ExecutionPolicy Bypass -File tools/test_root_motion.ps1`.
The tests exercise blend weights, coordinate conversion, jump preservation, and
floor contact without launching the game or loading character assets.
