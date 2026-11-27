# BMI renderer-private native spatial upscaler bridge

This source drop-in targets the inspected UE4.27 reference fork and belongs
inside `Engine/Source/Runtime/Renderer/Private/`, not in the runtime plugin.

Files:
- `BMISpatialUpscaler.h`
- `BMISpatialUpscaler.cpp`
- `patches/0001-register-bmi-spatial-upscaler.patch`

Integration:
1. Copy both source files into `Engine/Source/Runtime/Renderer/Private/`.
2. Apply the registration patch to `Renderer.cpp`. It adds the private header include
   and calls `RegisterBMISpatialUpscaler()` once from `FRendererModule::StartupModule()`.
3. Confirm the target engine source matches the pinned reference before applying.
4. Build the exact Bright Memory Android engine target and validate Vulkan output
   before enabling the runtime CVar.

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

Validation status: bridge and exact reference integration patch authored.
Neither has been compiled against the Bright Memory v1.34 engine snapshot or
tested on Android. No modified APK exists.
