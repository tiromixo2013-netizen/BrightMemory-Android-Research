# Bright Memory v1.34 — exact programmatic placement of the agreed graphics options

Target: Android v1.34 ARM64, UE4.27-derived build, Samsung Galaxy M14 4G / Adreno 610.
Branch: work/ue427-graphics-runtime-integration.

This file is an implementation map, not a claim that the options have already been injected into the APK.

## The exact three places each option must touch

1. **Original in-game menu (UMG):** preserve and extend the shipped graphics screen assets:
   - Parent: `BMI_Mobile/Content/UISystem_BMI/Menu/Seting/Graphics/BMI_Graphics_UMG.uasset/.uexp`
   - Existing resolution screen: `BMI_Graphics_Screen.uasset/.uexp`
   - Existing shadow and AO controls: `BMI_Graphics_Shadows.uasset/.uexp`, `BMI_Graphics_SSAO.uasset/.uexp`
   - Existing FPS control: `BMI_Graphics_FPSLimb_Android.uasset/.uexp`
   The new controls belong in this same Graphics menu, in the agreed order. They must not be put in a separate launcher, floating overlay, or external settings app.

2. **Settings persistence:** extend the existing `UserSave` SaveGame object and the same async load/save flow already referenced by the graphics widgets. Keep `BMI_Save` untouched; it stores progression/checkpoint data. Each new menu value must load into its control and save when changed.

3. **Runtime execution:** the widget calls BlueprintCallable functions in `UBMIGraphicsSettingsLibrary`; those functions dispatch to native runtime code and, where required, to the renderer. A setting is not complete merely because its widget, SaveGame field, CVar string, or Blueprint function exists.

## Option-by-option code placement

| Menu order | Option | Exact runtime location / behavior |
|---|---|---|
| 1 | Resolution Controller | Existing `BMI_Graphics_Screen` selection; preserve the shipped `r.MobileContentScaleFactor` choices (0.75, 0.85, 1.0, 1.2, 1.35, 1.5, 0). Native must resolve from actual display metrics and must not alter Android system DPI. |
| 2 | Graphics Quality | `UGameUserSettings` scalability plus the supported `sg.*Quality` / mobile CVars. Map Low, Medium, High, Very High, Ultra, Extreme to six distinct actual profiles; stock UE scalability levels 0–4 alone cannot make six distinct tiers. Extreme must set an explicit higher custom profile, not clamp to Ultra. |
| 3 | Render Scale | Use the game's native resolution-scale path: `UGameUserSettings::SetResolutionScaleValueEx` (observed symbol 0x8735b40), then `FSceneRenderer::ApplyResolutionFraction` (0x79e5ba0) / desired internal buffer sizing. Keep this independent from the menu's coarse Resolution Controller. |
| 4 | Upscaling Method | The real selection point is the renderer's mobile post-process upscale stage: `AddMobilePostProcessingPasses` (0x78870c0) and `ISpatialUpscaler::AddDefaultUpscalePass` (0x78fae30). The selected method must install its own valid render-graph pass. |
| 5 | Upscale Sharpness | Pass the value into the selected algorithm's shader parameters at that same upscale pass. It must not always write an SGSR1 CVar when another algorithm is selected. |
| 6 | Unified Upscale Preset | One shared preset selector. It maps through the currently selected upscaler to that method's supported render fraction and quality/sharpness parameters. It must not silently overwrite Graphics Quality or create separate preset menus. |
| 7 | AMD CAS | A separate FidelityFX CAS shader pass in the renderer's post-process graph, normally after the selected upscaler. Its enable state controls pass insertion; no registered pass means the option stays unavailable. |
| 8 | CAS Sharpness | A CAS shader parameter consumed by the CAS pass. It is not the same control as the selected upscaler's own sharpness. |
| 9 | Frame Generation | A real interpolation stage between rendered source frames and presentation. It needs motion vectors / depth or a validated motion-estimation path, history buffers, synchronization and pacing. x2/x3/x4 are multipliers of generated output, not repeated copies of the last frame. No verified UE4.27 + Adreno 610 implementation is currently present, so do not expose it as working yet. |
| 10 | Frame Pacing | Integrate with the active Android RHI's actual frame pacer and presentation path. The inspected build's device profile disables Vulkan (`r.Android.DisableVulkanSupport=1`); therefore the current Vulkan-only CVar implementation is not a valid target path. Trace the active OpenGL ES / Android frame-pacer path and its SwappyGL integration before wiring the toggle. |
| 11 | Low Latency | Runtime thread/queue synchronization and present scheduling. `r.OneFrameThreadLag=0` is the known UE4 thread-lag control; the proposed “Ultra / No Queue” must remain disabled until the actual active RHI queue depth can be changed safely. Never call `vkQueueWaitIdle` every frame. |
| 12 | Performance HUD | A small in-game UMG/native overlay fed by frame-time samples from `FPerformanceTrackingSystem::AnalyzeFrame` (0x852dd6c) and `GAverageFPS`; calculate 1% low from a rolling frame-time history. CPU/GPU utilization and GPU frame time require separate Android/vendor counters; show N/A when unavailable. |
| End | FPS Limit | Keep the existing Android FPS widget semantics and call `UGameUserSettings::SetFrameRateLimit` (0x87359dc) / the platform `FAndroidPlatformRHIFramePacer::GetFramePace` path. Only show rates confirmed by the device and display mode. |

## Upscaler list and one shared preset

Methods, in order: SGSR1, SGSR2, MWSR, FSR Mobile/Lite, Nearest, Bilinear.

- SGSR1: Qualcomm UE4.27 source exists in the repository; it still must be built into the exact engine and registered in the game's real mobile upscale pass.
- SGSR2: use the actual Qualcomm SGSR2 implementation; temporal history and motion-vector availability must be proven. Never relabel SGSR1 as SGSR2.
- MWSR: adapt the vendored HLSL to the target mobile shader compiler and RDG pass; the HLSL file alone is not runtime integration.
- FSR Mobile/Lite: integrate the mobile spatial EASU/RCAS implementation and required helpers as a real pass; this is not FSR 2.2.
- Nearest/Bilinear: implement as real sampling passes in the same upscale stage, not only as a changed CVar.
- Presets: Ultra Performance, Performance, Balanced, Quality, Ultra Quality. Their mapping is method-specific and must update actual input/output render dimensions and method parameters.

## Required call chain

`UMG selection event -> UserSave value -> UBMIGraphicsSettingsLibrary BlueprintCallable API -> native runtime validation -> GameUserSettings/CVar OR renderer RDG pass -> GPU command submission/present`.

On load, reverse the first two steps: read UserSave, restore the widget selection, then apply the runtime value. Reject unsupported methods rather than silently pretending they were applied.

## Current evidence boundary

- Original graphics UMG and SaveGame assets have been identified in the extracted v1.34 package.
- Existing screen-scale, shadow, AO and FPS CVar strings were found in the shipped widgets.
- Native UE4 symbols for resolution scaling, spatial upscaling, Android frame pacing and frame timing were identified.
- The repository contains a Blueprint-callable settings bridge and source/reference material for some upscalers.
- The extension is not compiled into the game's `libUE4.so`; the cooked UMG assets have not been safely rewritten; no modified APK or on-device validation exists.
- Vulkan is disabled in the inspected Android device profile. Any Vulkan-only implementation must be replaced or proven irrelevant to the active path.

**Acceptance rule:** do not show a control in the final game menu until its UMG event, saved value, runtime implementation, and visible/measurable effect all work.