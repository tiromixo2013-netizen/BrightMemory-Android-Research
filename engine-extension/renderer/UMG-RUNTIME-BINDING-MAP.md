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

| Menu setting | Blueprint function |
|---|---|
| Resolution Controller | `UBMIGraphicsSettingsLibrary::SetResolutionController` |
| Graphics Quality | `UBMIGraphicsSettingsLibrary::SetGraphicsQuality` |
| Render Scale | `UBMIGraphicsSettingsLibrary::SetRenderScale` |
| SGSR1 | `UBMIGraphicsSettingsLibrary::SetUpscaler` (method 0) |
| Upscale Sharpness | `UBMIGraphicsSettingsLibrary::SetUpscaleSharpness` |
| Upscale Preset | `UBMIGraphicsSettingsLibrary::SetUpscalePreset` |
| AMD CAS | `UBMIGraphicsSettingsLibrary::SetAMDCAS` |
| CAS Sharpness | `UBMIGraphicsSettingsLibrary::SetCASSharpness` |
| Low Latency | `UBMIGraphicsSettingsLibrary::SetLowLatency` |
| Frame Generation Method | `UBMIGraphicsSettingsLibrary::SetFrameGenerationMethod` |
| Frame Generation Multiplier | `UBMIGraphicsSettingsLibrary::SetFrameGenerationMultiplier` |

Frame generation routing is backend-aware: AFME is restricted to x2; an x3/x4
request while AFME is selected is routed to GLES only if that exact GLES
multiplier has been registered by a complete renderer adapter. Otherwise the
request is rejected without pretending the feature is active.

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
