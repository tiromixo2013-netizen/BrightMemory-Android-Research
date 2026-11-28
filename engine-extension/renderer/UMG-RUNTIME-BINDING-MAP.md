# Bright Memory v1.34 UMG runtime binding map

## Extracted target

The v1.34 Oodle extraction artifact contains these cooked widget assets:

- Graphics settings widget: `/Game/UISystem_BMI/Menu/Seting/Graphics/BMI_Graphics_UMG`
  (generated class `BMI_Graphics_UMG_C`).
- Parent settings menu: `/Game/UISystem_BMI/Menu/MainMenu/Two/BMI_Menu_Seting_UMG`
  (generated class `BMI_Menu_Seting_UMG_C`).

The graphics widget references the game's existing graphics children:
`BMI_GamePlay_AdvGraphics_TexBox`, `BMI_GamePlay_DisPlay_TexBox`,
`BMI_Graphics_FANSHE`, `BMI_Graphics_FOV`,
`BMI_Graphics_FPSLimb_Android`, `BMI_Graphics_Screen`,
`BMI_Graphics_Shadows`, and `BMI_Graphics_SSAO`.

## Runtime API bindings

The plugin exposes these Blueprint-callable functions for the agreed minimal
menu. Each setting's canonical mapping is also recorded in
`config/minimal-experimental-sgsr1-cas.json`.

| Order | UMG control (recommended stable name) | Event / value | Blueprint function |
|---:|---|---|---|
| 1 | `Combo_ResolutionController` | OnSelectionChanged; 0 Native, 1 540P, 2 640P, 3 720P, 4 900P, 5 980P, 6 1080P | `SetResolutionController(int32)` |
| 2 | `Combo_GraphicsQuality` | OnSelectionChanged; 0 Very Low through 6 Extreme | `SetGraphicsQuality(int32)` |
| 3 | `Slider_RenderScale` | OnValueChanged; finite 50..150 percent | `SetRenderScale(float)` |
| 4 | `Label_Upscaler` / fixed selector | SGSR1 only; send index 0 once on menu initialization | `SetUpscaler(0)` |
| 5 | `Slider_UpscaleSharpness` | OnValueChanged; 0..1 | `SetUpscaleSharpness(float)` |
| 6 | `Combo_UpscalePreset` | OnSelectionChanged; 0 Ultra Performance, 1 Performance, 2 Balanced, 3 Quality, 4 Ultra Quality | `SetUpscalePreset(int32)` |
| 7 | `Toggle_AMDCAS` | OnCheckStateChanged; disabled until real CAS pass is registered | `SetAMDCAS(bool)` |
| 8 | `Slider_CASSharpness` | OnValueChanged; 0..1; disabled with CAS | `SetCASSharpness(float)` |
| 9 | `Combo_LowLatency` | OnSelectionChanged; 0 Off, 1 On, 2 Ultra (Ultra disabled) | `SetLowLatency(int32)` |
| 10 | `Combo_FrameGenerationMethod` | OnSelectionChanged; 0 Off, 1 AFME, 2 GLES reprojection (non-Off disabled until backend registration) | `SetFrameGenerationMethod(int32)` |
| 11 | `Combo_FrameGenerationMultiplier` | OnSelectionChanged; 2 x2, 3 x3, 4 x4 (disabled until backend registration) | `SetFrameGenerationMultiplier(int32)` |

All functions are static BlueprintCallable entry points on
`UBMIGraphicsSettingsLibrary`. The returned bool currently reports whether
the command was dispatched by UE, not proof that the requested renderer feature
was applied; backend-specific runtime gates and logs remain authoritative.

Frame generation routing is backend-aware: AFME is restricted to x2; an x3/x4
request while AFME is selected is routed to GLES only if that exact GLES
multiplier has been registered by a complete renderer adapter. Otherwise the
request is rejected without pretending the feature is active.

## Runtime interaction contract

Preset mapping is shared and fixed: Ultra Performance = 50% / SGSR1 sharpness 1.90; Performance = 60% / 1.72; Balanced = 72% / 1.52; Quality = 86% / 1.30; Ultra Quality = 100% / 1.12. The UI should set its initial slider/combo values from the persisted game settings, then send only user changes (except the fixed SGSR1 selector initialization).

- Selecting SGSR1 enables and verifies the Qualcomm CVar, disables the optional BMI dispatcher and UE temporal upsampling.
- Selecting a preset uses the shared render-percentage helper and applies/verifies the official SGSR1 sharpness CVar.
- Manual Render Scale uses the same helper but does not change preset selection or sharpness.
- Manual Upscale Sharpness maps the UI range 0..1 to the SGSR1 shader range 1..2 and verifies readback.
- The Blueprint API rejects alternate upscaler indices; this profile is SGSR1-only.

## Integration boundary

The extracted files are cooked `.uasset/.uexp` binaries, not editable UMG
source or a UE project. The current runtime module and Blueprint API are
implemented, but the original `BMI_Graphics_UMG_C` event graph has not yet
been rebound to these functions. Completing that last UI step requires opening
the matching v1.34 assets in the exact UE4.27.2 project/editor, adding the
controls in the agreed order, binding their change events to the functions
above, cooking the modified assets, and verifying the menu in the game.

Do not label the in-game menu as integrated until that asset rebuild and
runtime verification have succeeded. Likewise, the API's presence does not
make unavailable renderer passes (SGSR1 adapter, AMD CAS, GLES frame
generation) operational; their existing capability gates must remain closed
until their implementations are compiled and validated.
