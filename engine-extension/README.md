# Bright Memory Android — UE4.27 runtime extension

This is the first native source-side implementation layer for the Bright Memory v1.34 graphics work. It targets the public UE4.27 source fork:

- https://github.com/jsuk10/UE4.27plus_Custom

## Implemented in this module

The runtime module registers console commands that call existing UE4 settings/CVar paths:

| Command | Engine path |
|---|---|
| `bmi.SetRenderScale 75` (percent) | `UGameUserSettings::SetResolutionScaleValueEx` (50..150 percent) |
| `bmi.SetGraphicsQuality 0..4` | `UGameUserSettings::SetOverallScalabilityLevel` + `ApplySettings` |
| `bmi.SetGraphicsPreset 0..4` | Eight native UE4.27 scalability groups + `ApplySettings` |
| `bmi.SetFrameLimit 30|45|60|90|120` | `UGameUserSettings::SetFrameRateLimit` + `ApplySettings` |
| `bmi.SetShadowQuality 0|1` | Existing `r.ShadowQuality` CVar |
| `bmi.SetMobileAO 0|1` | Existing `r.Mobile.AmbientOcclusionQuality` CVar |

These are native engine controls, not UI-only toggles. Render scale uses the engine's percentage-based ResolutionQuality API (not a 0..1 factor). They are intentionally kept separate from the game's cooked UMG assets at this stage.

## Renderer integration research

The UE4.27 reference source confirms the scene-view extension lifecycle, the primary spatial-upscaler slot, the `ISpatialUpscaler::AddPasses` RDG contract, and screen-pass helpers. However, the upscaler interface is declared in a **Renderer private header** (`Renderer/Private/PostProcess/PostProcessUpscale.h`). That means this cannot honestly be shipped as an ordinary public-header-only plugin without an explicit engine-side integration decision.

See [RENDERER-INTEGRATION-CONTRACT.md](RENDERER-INTEGRATION-CONTRACT.md) for verified signatures, ownership constraints, and the safe next implementation boundary.

## Not falsely enabled

SGSR1, SGSR2, MWSR, FSR Lite, AMD CAS, frame generation, low-latency queue changes and the performance HUD are not registered as working controls here. They require renderer/shader integration and validation against the target game's exact UE build and Android Vulkan path. The original game binary does not prove those algorithms are already compiled in.

Do not add guessed offsets or call `vkQueueWaitIdle` every frame.

## Integration

Copying this plugin beside an already-shipped APK does not inject code into `libUE4.so`. The native game target must link the module and cook any shader permutations it needs.

## Validation state

- Source code added: yes.
- Renderer interface signatures inspected in the reference UE4.27 fork: yes.
- Renderer implementation compiled against the Bright Memory-matched engine target: no.
- Installed or tested on Samsung Galaxy M14 4G: no.
- Modified APK produced: no.

Do not expose menu controls for uncompiled features.
