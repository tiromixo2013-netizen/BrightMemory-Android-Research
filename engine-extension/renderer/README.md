# BMI renderer integration

## Unified spatial dispatcher

`BMIUnifiedSpatialUpscaler.h/.cpp` define one renderer-private UE4.27
`ISpatialUpscaler` implementation. It owns the spatial dispatch point and
chains optional post-reconstruction stages in this order:

1. Selected spatial reconstruction backend.
2. Optional AMD CAS pass, only when a real pass is registered.
3. Optional performance HUD compositor, only when a real pass is registered.

Frame Generation is deliberately separate; it is not a spatial upscaler pass.
The dispatcher currently contains only UE4's built-in Nearest and Bilinear
fallbacks. External methods are accepted by the registry API but none of the
four external adapters is registered yet.

The view extension is opt-in: `bmi.Pipeline.Enabled` defaults to 0. The
Renderer startup patch replaces the old BMI registration call with
`RegisterBMIUnifiedSpatialUpscaler()`, which retains/registers the BMI view
extension. During `BeginRenderViewFamily`, BMI claims the primary slot only
when the pipeline is enabled, the selected method is registered, and the
family has no primary spatial upscaler. UE4.27's setter is assign-once and
asserts on a second assignment, so BMI never overwrites another extension.

The vendored Qualcomm SGSR extension also assigns the primary slot only when
it is empty. Its `r.Qualcomm.SGSR.Enabled` CVar must therefore be disabled
before BMI claims the slot; `SetUpscaler` explicitly disables that CVar
(and the SGSR2 CVar if present) after validating BMI's method mask. If another
extension claims the primary slot first, BMI safely leaves it untouched and
will not become active for that view family.

Registration patch:
`patches/0001-register-bmi-spatial-upscaler.patch`.

The patch targets the inspected reference source
`jsuk10/UE4.27plus_Custom`. That reference is not proven identical to the
Bright Memory v1.34 engine snapshot. The patch is a reference integration
change, not proof it has been applied to or compiled with the game's engine.

## Backend source status

- **SGSR1:** Qualcomm UE4.27 plugin source is vendored under `vendor/SGSR1/`.
  It still needs to be adapted/registered as a BMI backend and compiled with
  the target engine. Its vendor view extension must remain disabled when BMI
  owns the primary slot.
- **SGSR2:** no verified SGSR2 source is present in this vendor tree. It must
  not be represented by an SGSR1 alias.
- **MWSR:** MIT HLSL source from `linckosz/moonlight-web/mwsr` is vendored.
  The shader uses `ddx_coarse/ddy_coarse`, a 96-byte constants block and a
  linear-clamp sampler. It requires an UE4.27 global-shader/RDG adapter and
  validation against the actual target RHI/compiler before registration.
- **FSR Mobile:** atyuwen's MIT optimized FSR1 source is vendored under
  `vendor/FSR-Mobile/`. It combines EASU reconstruction and an RCAS-style
  limiter/resolve in one pixel-shader function. The host must supply the
  sampling callback, EASU constants and AMD portability helpers. This is the
  only FSR option; do not add a duplicate generic FSR1 entry.
- **Nearest/Bilinear:** use UE4.27's built-in
  `ISpatialUpscaler::AddDefaultUpscalePass` path. These are the only methods
  represented in the current default registered-method mask.

## Optional stages

AMD FidelityFX CAS headers are vendored under `vendor/AMD-CAS/`, but there
is no registered CAS RDG shader pass. The runtime rejects enabling CAS while
the availability CVar is false. Do not replace CAS with
`r.Tonemapper.Sharpen`.

The HUD callback exists in the dispatcher contract, but there is no actual
telemetry compositor registered. Its availability remains false.

## Runtime and Blueprint settings

`Source/BMIGraphicsRuntime` contains the settings command bridge and
Blueprint-callable API. The menu schema is in
`../../config/graphics-settings.json`. The agreed order is Resolution
Controller, Graphics Quality, Render Scale, Upscaling Method, Upscale
Sharpness, shared Upscale Preset, AMD CAS, CAS Sharpness, Frame Generation,
Frame Pacing, Low Latency, and Performance HUD.

The Blueprint library is an API seam, not a completed in-game menu. The
Bright Memory v1.34 UMG assets/source are not integrated here, and the plugin
has not been compiled against the shipped engine snapshot.

## Target constraints and verification

The inspected target profile disables Vulkan and uses Android OpenGL ES. The
public UE4.27 reference confirms the general RDG screen-pass API shape, but
does not prove that the shipped game uses an identical renderer snapshot.
Consequently Vulkan-only frame-pacing CVars are not presented as a working
solution. The active GLES/Swappy pacing path still needs integration. FSR
Mobile remains source-only: its gather callbacks, half-precision path, shader-
directory mapping, and UE shader compiler output must be checked against the
game's exact GLES target before an adapter can be registered. Low Latency On
currently maps to UE4's `r.OneFrameThreadLag=0`; Ultra/No Queue is rejected
because no supported active-RHI queue-depth control is verified.

Run `python scripts/validate_bmi_runtime.py` for source/schema consistency.
That is a static source check only. It does not compile UE shaders, prove
renderer ABI compatibility, produce an APK, or replace an Adreno 610 device
test. No modified APK or on-device success is claimed.
