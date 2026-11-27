# BMI renderer integration

## Native bridge

`BMISpatialUpscaler.h/.cpp` provide the renderer-private UE4.27 upscaler bridge.
The bridge currently routes to UE4.27's built-in Nearest/Bilinear passes.
Registration patch:
`patches/0001-register-bmi-spatial-upscaler.patch`.

The bridge defaults to disabled (opt-in) and does not replace an already-installed primary spatial upscaler supplied by the game or another extension. It must be copied into the exact
engine's `Renderer/Private` directory and registered from
`FRendererModule::StartupModule()`. This source has not been compiled against
the shipped Bright Memory v1.34 engine snapshot.

## SGSR1 source

The official Qualcomm UE4.27 SGSR1 plugin source and shader files are vendored
under `vendor/SGSR1/` with upstream BSD-3-Clause notices. The descriptor is
named `SGSR.uplugin` because Unreal resolves `FindPlugin("SGSR")` from the
descriptor basename. Install its contents under `Plugins/SGSR/`.

Startup checks the plugin handle and `Shaders/` directory before registering
`/Plugin/SGSR`, and logs a safe error if either is missing. SGSR1 defaults to
disabled, does not replace an already-installed primary upscaler, and forwards
the runtime `r.Qualcomm.SGSR.Sharpness` CVar into the pixel shader. The slider
maps 0..1 to SGSR1's 1..2 edge-sharpness range.

This is vendored source, not a compiled plugin in the shipped APK. It still
requires installation in the matching engine, project enablement, shader
cooking, engine compilation, and Android Vulkan validation.

## AMD CAS source

Official AMD FidelityFX CAS portability and algorithm headers are vendored
under `vendor/AMD-CAS/` with their MIT notices. They are not yet connected to
a CAS RDG pass or the game's post-process chain. The runtime bridge refuses
to claim CAS is enabled when the renderer pass CVar is absent. Do not replace
the CAS algorithm with `r.Tonemapper.Sharpen`.

## Blueprint settings API

The runtime plugin includes
`Source/BMIGraphicsRuntime/Public/BMIGraphicsSettingsLibrary.h` and its
implementation in `Private/BMIGraphicsSettingsLibrary.cpp`. The static
`UBMIGraphicsSettingsLibrary` functions are exposed to Blueprint under
**Bright Memory | Graphics**, allowing a UMG settings widget to call the
runtime commands without embedding console strings in Blueprint graphs.

This is an API seam, not a completed in-game menu. The Bright Memory v1.34
settings-widget assets/source are not present in this repository, and the
library and renderer sources have not been compiled against the exact Bright Memory v1.34 engine snapshot. The
controls are not yet visible in the shipped game's settings menu.

## Remaining implementation and verification

- SGSR2 requires temporal history and motion-vector integration.
- MWSR and FSR Lite do not yet have verified UE4.27 renderer implementations.
- AMD CAS needs a real RDG pass and shader integration after tonemapping.
- Frame pacing requires a validated Android Vulkan/Swappy control path.
- Frame generation and Performance HUD are not implemented.
- Low Latency On maps to UE4's `r.OneFrameThreadLag=0`; Ultra/No Queue remains
  rejected because no supported Vulkan queue-depth control is verified.
- Build the exact Android target, run the source validator, and test on-device
  before exposing controls. No modified APK or device test is claimed.
