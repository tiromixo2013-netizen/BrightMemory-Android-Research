# Bright Memory: Infinite Android v1.34 — config/UI extraction findings

Date: 2026-11-27  
Source APK: v1.34 ARM64, SHA-256 `c35b05ac7b19fd087844de2b6261d33fbe103a6a5714c5e42d6101fab07881e4`  
Extraction workflow: `45918390607`  
Artifact: `brightmemory-v1.34-config-ui-extract`

## Extraction result

The targeted extraction completed successfully. It recovered five project configuration files and 406 UAsset + 406 UExp files selected from likely UI folders. The selected UI assets are mostly HUD, weapon, mission, and gameplay widgets; the current filter did **not** identify a confirmed graphics-settings screen/widget. This is a limitation of the targeted filter, not proof that no such asset exists.

## Confirmed project configuration

### DefaultEngine.ini

Relevant renderer defaults include:

- `r.MobileMSAA=1`
- `r.DefaultFeature.Bloom=True`
- `r.DefaultFeature.AmbientOcclusion=True`
- `r.DefaultFeature.AutoExposure=False`
- `r.DefaultFeature.MotionBlur=False`
- `r.TemporalAA.Upsampling=False`
- `r.TemporalAA.Upscaler=False`
- `r.DefaultFeature.AntiAliasing=2`
- `r.MobileHDR=True`
- `r.Mobile.AmbientOcclusion=True`
- `r.Mobile.SupportsGen4TAA=False`
- `r.Tonemapper.Sharpen=1.35`
- `r.Tonemapper.Quality=2`
- `r.ViewDistanceScale=1`
- `FrameRateLock=PUFRL_60`
- Smooth frame-rate range is configured around 22–62 FPS.

These are packaged defaults. They do not prove that changing a config at runtime will be honored by this shipping build, nor do they expose a complete menu binding.

### DefaultDeviceProfiles.ini

Android profiles are present, including:

- Generic Android / Low / Mid / High / Default
- `Android_Adreno6xx` and `Android_Adreno6xx_Vulkan`
- Multiple Mali profiles, including G71/G72/G76/G77/G78/G710 variants
- Android Vulkan SM5 and unknown Vulkan profiles

Generic Android profile explicitly sets `r.Android.DisableVulkanSupport=1`; Adreno6xx Vulkan profile overrides it to 0. Generic profile also sets `r.MobileContentScaleFactor=1.0`. Low/Mid/High profiles currently show scale factor 1.0/1.0/1.0 in the extracted config. Profile selection and actual device matching still need runtime validation.

### UserEngine.ini

Contains DLSS override settings only. It is not evidence that DLSS is active on Android.

## Implementation implications

1. **Resolution / render scale:** the project has mobile content scale and Unreal screen-percentage/scalability APIs in the native engine analysis. This is a plausible first implementation area, but the runtime call path and safe update point still need to be proven. Do not blindly edit packaged defaults and assume it works.
2. **Graphics quality:** Unreal scalability categories are available in the native engine. A real feature must map each user-facing quality level to actual supported CVar/scalability settings and confirm visible changes in-game.
3. **Upscalers:** the config does not identify SGSR1/SGSR2/MWSR/FSR Lite integrations. They require an actual render-pass implementation or compatible library integration; a config toggle cannot create them.
4. **AMD CAS:** no confirmed CAS implementation was established by this config extraction.
5. **Frame generation:** no valid game motion-vector/depth integration or real interpolation path has been established. Do not advertise x2/x3/x4 until an actual implementation is demonstrated.
6. **Frame pacing / low latency:** engine settings and native symbols are not sufficient by themselves; runtime queue and pacing behavior must be measured on-device.

## Important limitations / next investigation

- The extracted UMG selection did not confirm the game's graphics-settings widget. Next scan should broaden the PAK inventory for asset names and serialized string references containing settings, options, quality, resolution, frame rate, and related localized text.
- Inspect the complete Android device-profile inheritance and runtime profile selection before modifying profile values.
- Determine whether the shipping APK loads writable user config from its external files directory, and whether those values are read before renderer initialization.
- Inspect the native engine's call sites / relevant function usage before hooking any function.
- No modified APK has been built or tested as part of this extraction.

## Status

This is a successful **source/config extraction and analysis milestone**, not a completed graphics-mod implementation. Preserve the original APK unchanged.
