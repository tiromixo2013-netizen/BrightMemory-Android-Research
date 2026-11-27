# Native graphics integration readiness — Bright Memory: Infinite Android v1.34

Date: 2026-11-27

## Scope

Target: Bright Memory: Infinite Android v1.34, ARM64, UE4.27-family renderer.
Working engine source candidate: jsuk10/UE4.27plus_Custom at `2e6b53c74a8290ffefe72cf36ac2b051687cc544`.
Target APK SHA-256: `c35b05ac7b19fd087844de2b6261d33fbe103a6a5714c5e42d6101fab07881e4`.

This repository currently contains research/configuration, not the private BMI game project or a complete rebuildable Android UE project. The steps below are source integration preparation, not a claim that a modified APK has been built.

## Upstream Qualcomm source added as a pinned submodule

Repository: https://github.com/SnapdragonGameStudios/snapdragon-game-plugins-for-unreal-engine
Branch: `engine/4.27`
Pinned commit: `761e8393ae44a6cef48e51f37e315ccce494b055`
Submodule path: `third_party/qualcomm-ue-plugins`

The submodule retains upstream code and licenses; do not copy or rewrite upstream implementation files without preserving their BSD-3-Clause notices.

### SGSR1

The official UE4.27 branch contains:
- `Plugins/SGSR/SGSR1.uplugin`
- `Plugins/SGSR/Source/SGSRSpatialUpscaling/Private/SGSRSpatialUpscaler.cpp`
- `Plugins/SGSR/Source/SGSRSpatialUpscaling/Private/SGSRViewExtension.cpp`
- `Plugins/SGSR/Shaders/Private/PostProcessSGSR.usf`
- `Plugins/SGSR/Shaders/Private/sgsr.ush`

The source implements a real `ISpatialUpscaler::AddPasses` render-graph path and assigns it through `FSceneViewExtensionBase::BeginRenderViewFamily` using `SetPrimarySpatialUpscalerInterface`. This directly matches the candidate UE4.27 extension architecture identified in `ue427-source-dispatch-comparison.md`.

The upstream plugin's own module asserts UE4.27.1 or newer. The candidate source is UE4.27.2. This is a strong source compatibility match, not yet a successful build against the BMI target.

### SGSR2

The official UE4.27 branch contains SGSR2 variants including `2passNoAlpha`, `2pass_fs_taau`, and `3pass`.

The upstream README requires mobile TAA / temporal upsampling for the 2-pass TAA variant, and specifies project settings for Android packaging and device profiles. Bright Memory's shipped project configuration and temporal AA support must be checked before enabling SGSR2. It cannot be assumed that SGSR2 can coexist as a second primary upscaler with SGSR1: the renderer view-family primary interface is a single selected interface, so the menu must switch one algorithm at a time.

## Integration order and current implementation gates

The user-approved menu order remains:
1. Resolution Controller
2. Graphics Quality
3. Render Scale
4. Upscaling Method
5. Upscale Sharpness
6. Unified Upscale Preset
7. AMD CAS
8. CAS Sharpness
9. Frame Generation
10. Frame Pacing
11. Low Latency
12. Performance Overlay

| Feature | Source/implementation finding | Gate before enabling in game |
|---|---|---|
| Resolution Controller | Existing BMI screen UMG and `r.MobileContentScaleFactor` commands found | Map Native/540P–1080P to actual device display and game render dimensions; preserve Android DPI |
| Graphics Quality | UE4 `UGameUserSettings` scalability setters exist | Build six real mobile scalability profiles and validate each visual category |
| Render Scale | Native resolution scale setters and renderer fraction path exist | Wire UI to one scale path; no per-frame resource recreation |
| SGSR1 | Official UE4.27 plugin source and shader files available in pinned upstream submodule | Integrate modules, shaders, plugin config and cooked shader data into BMI build |
| SGSR2 | Official UE4.27 source variants available | Verify target TAA/motion-vector settings and package constraints; select compatible variant |
| MWSR | No authoritative UE4.27 implementation/source identified | Do not expose until source, license, shader and performance path are verified |
| FSR Lite | No official implementation under this exact name verified | Do not relabel FSR1 or fabricate a Lite mode; locate the agreed implementation or implement/test a distinct mobile path |
| Nearest / Bilinear | Can be implemented as real RDG spatial sampler passes | Add shader permutations and validate filtering/output extents |
| Upscale sharpness | Requires algorithm-specific shader parameters | Only expose for methods that implement a validated sharpness parameter |
| Unified presets | Must map to each selected method's supported input fraction/quality mode | Build per-algorithm mappings; do not assume identical ratios |
| AMD CAS | Requires a separate post-upscale RDG pass and CAS shader port | Integrate shader and validate ordering/quality; avoid double-sharpening |
| Frame Generation x2/x3/x4 | No verified UE4.27 + Adreno 610 implementation found | Keep disabled/hidden; requires real motion estimation/interpolation, frame ownership and pacing |
| Frame Pacing | Target binary contains Vulkan pacing and Android frame pacer symbols | Validate supported Vulkan present modes and pacing behavior on device |
| Low Latency | Vulkan queue/synchronization path exists | Measure queue depth and present latency; never force zero queue or insert per-frame `vkQueueWaitIdle` |
| Performance Overlay | Engine frame-duration tracking symbols found | Add rolling frame-time history; CPU/GPU/temp are optional Android/vendor telemetry and must show N/A if unavailable |

## Important architectural constraint

The UE4.27 candidate allows one primary spatial upscaler interface per view family. The integration must implement a single dispatcher/selection policy and must never register SGSR1 and SGSR2 as competing primary upscalers simultaneously. The selected implementation must own the render-graph pass and preserve correct `Fork_GameThread` lifetime semantics.

## Build/repack blocker

The public research repository does not contain:
- the BMI `.uproject` / game source project;
- BMI private plugin sources and build rules;
- the exact target project config/device profiles;
- a verified target-compatible cooked shader pipeline;
- a proven binary patch/ABI integration path into the shipped `libUE4.so`.

Therefore, adding upstream source is not equivalent to integrating it into the APK. Do not modify or repack the original APK until the game project/config and Android build pipeline are available and the target ABI is validated.

## Next executable work

1. Restore the six-part source package through the existing MEGA workflow and inspect its contents for project files, private plugins, Android config and shader sources.
2. Inspect the target's actual anti-aliasing / temporal-upscaling configuration and Android packaging settings.
3. Integrate SGSR1 first as the spatial baseline through the verified scene-view extension route.
4. Add SGSR2 as an alternate selectable implementation only if the shipped project supports its TAA prerequisites.
5. Add CAS and Nearest/Bilinear RDG passes; only then build the unified selector.
6. Implement and test the non-upscaler settings through native game APIs.
7. Keep FG, MWSR and FSR Lite hidden until their real implementations and compatibility are proven.
