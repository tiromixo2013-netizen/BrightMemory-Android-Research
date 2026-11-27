# BMI renderer integration

## Native bridge

`BMISpatialUpscaler.h/.cpp` provide the renderer-private UE4.27 upscaler bridge.
The bridge currently routes to UE4.27's built-in Nearest/Bilinear passes.
Registration patch:
`patches/0001-register-bmi-spatial-upscaler.patch`.

## SGSR1 source

The official Qualcomm UE4.27 SGSR1 plugin source and shader files are vendored
under `vendor/SGSR1/` with upstream BSD-3-Clause notices. The plugin descriptor
is `SGSR1.uplugin`; when installing into the engine it must be placed in a
folder named `SGSR`, because the upstream shader module resolves
`FindPlugin("SGSR")`.

The vendored source has these project-specific changes:
- SGSR1 defaults to disabled (opt-in).
- It does not replace an already-installed primary spatial upscaler.
- A runtime `r.Qualcomm.SGSR.Sharpness` CVar is forwarded into the pixel shader.
- The settings bridge maps the user's 0..1 sharpness slider to SGSR1's 1..2
  edge-sharpness range.

This is a source integration package, not a compiled plugin for the shipped
Bright Memory APK. It still needs to be placed in the matching UE4.27 engine,
enabled by the game project, and compiled with the game's exact shader settings.

## AMD CAS source

Official AMD FidelityFX CAS portability and algorithm headers are vendored
under `vendor/AMD-CAS/` with their MIT notices. They are not yet connected to
an RDG shader pass or the game's post-process chain. The settings bridge refuses
to report CAS as enabled if the renderer CVar/pass is absent.

## Runtime settings bridge

The module source provides commands:
- `bmi.SetResolutionController 0..6`: original game mobile scale values.
- `bmi.SetGraphicsQuality 0..4`: UE4 scalability groups.
- `bmi.SetRenderScale 50..150`: GameUserSettings resolution quality.
- `bmi.SetUpscalePreset 0..4`: Ultra Performance through Ultra Quality render-scale mapping.
- `bmi.SetUpscaler 0..5`: SGSR1, SGSR2, MWSR, FSR Lite, Nearest, Bilinear.
  Only SGSR1 (when the vendored plugin is linked) and Nearest/Bilinear have
  source routes; unavailable algorithms log an error instead of silently
  selecting a fake implementation.
- `bmi.SetUpscaleSharpness 0..1`: forwards to SGSR1 shader parameter when linked.
- `bmi.SetCAS 0|1` and `bmi.SetCASSharpness 0..1`: require the CAS RDG pass;
  currently they fail closed because that pass is not yet registered.
- `bmi.SetLowLatency 0|1|2`: Off, UE4 `r.OneFrameThreadLag=0` low-latency
  thread synchronization, or Ultra/No Queue. Ultra is rejected until the
  Vulkan queue-depth path is verified.

No per-frame `vkQueueWaitIdle`, guessed queue-depth manipulation, guessed
offsets, or vtable hooks are used.

## Validation boundary

These changes have not been compiled against the exact Bright Memory v1.34
engine snapshot and have not been tested on Android Vulkan. The game project
and matching engine build environment are not present in this repository.
No modified APK exists. All menu controls remain hidden until compile and
device validation pass.
