# Math core and transform ownership

`src/Framework/Core/Math` contains CPU-only value types and functions. It depends
on the standard library and DirectXMath, with no scene, Win32, or GPU resource
dependency. `tools/check_architecture.ps1` enforces this boundary.

## Coordinate convention

- Left-handed coordinates: +X right, +Y up, +Z forward.
- Angles are radians. `rotationRadians` stores pitch X, yaw Y, roll Z and uses
  DirectXMath's `XMMatrixRotationRollPitchYaw` convention.
- Row vectors multiply matrices on the right: local position × scale × rotation
  × translation. View-projection is view × projection.
- Model height normalization remains part of model loading. It is not repeated
  by the actor's world transform.
- The shader boundary transposes CPU matrices for HLSL's column-major storage.

```cpp
Transform transform;
transform.position = { 2.0f, 0.0f, 3.0f };
transform.rotationRadians.y = DirectX::XM_PIDIV2;
transform.scale = { 2.0f, 1.0f, 1.0f };

auto worldPoint = transform.TransformPoint({ 1.0f, 0.0f, 0.0f });
auto worldDisplacement = transform.TransformDirection({ 1.0f, 0.0f, 0.0f });
DirectX::XMFLOAT3 worldNormal;
bool hasNormal = transform.TryTransformNormal({ 0.0f, 1.0f, 0.0f }, worldNormal);
```

`TransformPoint` includes translation. `TransformDirection` applies the linear
rotation-and-scale portion without translation or normalization, so it can also
transform displacement. `TryTransformNormal` applies the inverse-transpose
linear matrix and normalizes the result; it returns false with a zero output for
an invalid normal or a singular/nonfinite transform.

`MathUtils::TryCreateNormalMatrix` returns false with an identity output if no
finite inverse can be represented. The textured render pipelines use that
identity fallback for a degenerate world transform. Normals and tangents have
separate shader transforms: inverse-transpose for normals, world linear transform
for tangents. Lighting is then evaluated consistently in world space.

## Ownership and existing gameplay

`StaticMeshActor` and `SkinnedMeshActor` each own one `Transform`. Their existing
position and yaw APIs operate on it; `GetTransform()` provides read-only access.
`StaticModel` and `SkinnedModel` no longer store a duplicate world position or yaw.
They receive the world matrix through `Draw(renderer, world)`; the editor supplies
its own placement the same way.

Existing upright collision actors still use their original position/yaw-only
controls. Arbitrary pitch/roll/scale setters are not exposed on them because their
collider shapes do not yet support those changes. Skinned collision anchors remain
horizontal offsets above the actor's bottom height.

Root-motion translation uses the shared direction transform. Its Ignore/Blend/Apply
configuration, vertical opt-in, and programmatic jump protection remain as described
in [root-motion.md](root-motion.md).

## Numerical contracts

- `TryNormalize`: failure clears output; the default minimum length is 1e-6 world
  units. It handles nonfinite and extreme finite input values, including aliased
  input/output. Pass epsilon 0 for an exact-zero cutoff.
- `TryNormalizeXZ`: ignores Y and defaults to an exact-zero cutoff, preserving
  the previous movement-input behavior.
- `NormalizeAngle`: canonical range [-pi, pi); nonfinite input becomes zero.
- `LerpAngle`: shortest-path interpolation, using the same wrap convention.
- `SmoothAmount`: exponential smoothing with stable small-time-step arithmetic;
  nonpositive/nonfinite parameters return zero.

The camera and CPU skinning call these helpers instead of private copies.

## Verification

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/test_math_core.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools/test_root_motion.ps1
```

The math tests compile only the math core. They cover SRT order, point/direction
separation, nonuniform-scale normals, invalid inputs, and angle/smoothing edges.
The existing root-motion tests protect jump trajectories and landing behavior.

The editor queues model-open requests until before the next `BeginFrame`, prepares
replacement state, and waits for submitted GPU work before releasing the old model.
A failed replacement load preserves the current model and event document.
