# Bright Memory Android — UE4.27 runtime extension

This is the first native source-side implementation layer for the Bright Memory v1.34 graphics work. It targets the public UE4.27 source fork:

- https://github.com/jsuk10/UE4.27plus_Custom

## Implemented in this module

The runtime module currently registers the following source-level commands:

| Command | Intended UE4 path | Verification boundary |
|---|---|---|
| `bmi.SetRenderScale 75` (percent) | `UGameUserSettings::SetResolutionScaleValueEx` (50..150 percent) | Source added; target engine build not compiled |
| `bmi.SetGraphicsQuality 0..6` | Eight UE4 scalability groups + supported mobile CVar overrides | Source added; per-tier visual effects not device-tested |
| `bmi.SetFrameLimit 30|45|60|90|120` | `UGameUserSettings::SetFrameRateLimit` + `ApplySettings` | Source added; device-supported rates not tested |
| `bmi.SetResolutionController 0..6` | Existing `r.MobileContentScaleFactor` path | Source added; display mapping not device-tested |
| `bmi.SetFramePacing 0|1` | Vulkan CPU fallback frame-pacer CVars | Disabled on the active Android OpenGL ES target; enable request is rejected |
| `bmi.SetLowLatency 0|1|2` | `r.OneFrameThreadLag` | Source added; Ultra/no-queue mode is rejected |

The Blueprint library forwards calls to these commands. SGSR1 selection, preset, sharpness, and Render Scale now share verified runtime helpers and readbacks. The Blueprint bool return reports command dispatch, not renderer success. This is still source code only: it has not been compiled into the game's native library or rebound into the cooked settings menu.

The earlier draft documentation mentioned `SetGraphicsPreset`, eight individual scalability groups, `SetShadowQuality`, and `SetMobileAO` as implemented commands. Those command implementations are not present in the current runtime module and must not be treated as available.

## Renderer integration research

The UE4.27 reference source confirms the scene-view extension lifecycle, the primary spatial-upscaler slot, the `ISpatialUpscaler::AddPasses` RDG contract, and screen-pass helpers. However, the upscaler interface is declared in a **Renderer private header** (`Renderer/Private/PostProcess/PostProcessUpscale.h`). That means this cannot honestly be shipped as an ordinary public-header-only plugin without an explicit engine-side integration decision.

See [RENDERER-INTEGRATION-CONTRACT.md](RENDERER-INTEGRATION-CONTRACT.md) for verified signatures, ownership constraints, and the safe next implementation boundary.

## Not falsely enabled

The active profile selects the vendored Qualcomm SGSR1 plugin through its runtime enable CVar and writes its real shader sharpness parameter. This selection path is implemented in source but is not yet compiled into the game's native library or verified on device. SGSR2, MWSR, FSR Lite, AMD CAS, frame generation, low-latency queue changes and the performance HUD must remain unavailable unless their respective renderer/backend capability is registered and validated. The inspected target is Android OpenGL ES, not Vulkan. The original game binary does not prove those algorithms are already compiled in.

Do not add guessed offsets or call `vkQueueWaitIdle` every frame.

## Integration

Copying this plugin beside an already-shipped APK does not inject code into `libUE4.so`. The native game target must link the module and cook any shader permutations it needs.

## Validation state

- Source code added: yes.
- Renderer interface signatures inspected in the reference UE4.27 fork: yes.
- Renderer implementation compiled against the Bright Memory-matched engine target: no.
- SGSR1 runtime bridge source linked and readback-guarded: yes; target module build/runtime proof: no.
- Cooked UMG event graph rebound to Blueprint functions: no.
- Installed or tested on Samsung Galaxy M14 4G: no.
- Modified APK produced: no.

Do not expose menu controls for uncompiled features.
