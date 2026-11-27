# Bright Memory Android — UE4.27 runtime extension

This is the first native source-side implementation layer for the Bright Memory v1.34 graphics work. It targets the public UE4.27 source fork:

- https://github.com/jsuk10/UE4.27plus_Custom

## Implemented in this module

The runtime module registers console commands that call existing UE4 settings/CVar paths:

| Command | Engine path |
|---|---|
| `bmi.SetRenderScale 0.75` | `r.MobileContentScaleFactor` |
| `bmi.SetGraphicsQuality 0..4` | `UGameUserSettings::SetOverallScalabilityLevel` + `ApplySettings` |
| `bmi.SetFrameLimit 30|45|60|90|120` | `UGameUserSettings::SetFrameRateLimit` + `ApplySettings` |
| `bmi.SetShadowQuality 0|1` | Existing `r.ShadowQuality` CVar |
| `bmi.SetMobileAO 0|1` | Existing `r.Mobile.AmbientOcclusionQuality` CVar |

These are native engine controls, not UI-only toggles. They are intentionally kept separate from the game's cooked UMG assets at this stage.

## Not falsely enabled

SGSR1, SGSR2, MWSR, FSR Lite, AMD CAS, frame generation, low-latency queue changes and the performance HUD are not registered as working controls here. They require renderer/shader integration and validation against the target game's exact UE build and Android Vulkan path. The original game binary does not prove those algorithms are already compiled in.

The next renderer patch uses UE4.27's ISceneViewExtension / primary spatial-upscaler seam. Do not add guessed offsets or call vkQueueWaitIdle every frame.

## Integration

Copy this plugin into the game's project plugin directory and enable it in the project descriptor before building the matching Android target. A plugin cannot be injected into an already-shipped libUE4.so by copying the plugin folder beside the APK; the native game target must link the module and cook any shader permutations it needs.

## Validation state

- Source code added: yes.
- Compiled against the Bright Memory-matched engine target: no.
- Installed or tested on Samsung Galaxy M14 4G: no.
- Modified APK produced: no.

Do not expose menu controls for uncompiled features.
