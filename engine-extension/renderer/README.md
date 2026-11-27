# BMI renderer-private native spatial upscaler bridge

This source drop-in targets the inspected UE4.27 reference fork and belongs
inside `Engine/Source/Runtime/Renderer/Private/`, not in the runtime plugin.

Files:
- `BMISpatialUpscaler.h`
- `BMISpatialUpscaler.cpp`

Integration:
1. Add both files to the Renderer module's private source tree.
2. Include `BMISpatialUpscaler.h` in the Renderer module implementation.
3. Call `RegisterBMISpatialUpscaler()` once from `FRendererModule::StartupModule()`.
4. Build the exact Bright Memory Android engine target before enabling the CVars.

Runtime controls:
- `bmi.UseNativeSpatialUpscaler 0|1` (default 0; off preserves original path)
- `bmi.NativeUpscaleMethod 0|1` (0 Nearest, 1 Bilinear)

This deliberately reuses UE4.27's existing `ISpatialUpscaler::AddDefaultUpscalePass`
and the renderer's existing shaders. It does not claim to implement SGSR, FSR,
MWSR, CAS, frame generation, frame pacing, or low latency. The inspected
reference shader only guarantees Nearest/Bilinear permutations for mobile;
other built-in methods are therefore not exposed by this bridge.

Safety:
- It only installs when explicitly enabled.
- It does not replace a primary upscaler already installed by the game.
- The per-view-family upscaler is allocated for the view-family ownership
  contract and recreated by `Fork_GameThread`.
- No guessed offsets, vtable hooks, or per-frame queue stalls are used.

Validation status: source authored against the pinned reference API; not yet
compiled against the Bright Memory v1.34 engine snapshot or tested on Android.
