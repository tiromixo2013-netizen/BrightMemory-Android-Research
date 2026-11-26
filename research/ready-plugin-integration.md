# Ready Upscaler Plugin Integration — Bright Memory Android

Date: 2026-11-26

Target: Bright Memory: Infinite Android v1.34, ARM64, UE4.27Plus / candidate UE4.27.2.

## Verified ready-made UE4.27 integrations

### Qualcomm SGSR1

Source branch: https://github.com/SnapdragonGameStudios/snapdragon-game-plugins-for-unreal-engine/tree/engine/4.27/Plugins/SGSR

Plugin descriptor: `SGSR1.uplugin` (runtime modules `SGSR` and `SGSRSpatialUpscaling`; Android is whitelisted).

Integration pattern observed in Qualcomm source:
- `FSGSRModule::StartupModule()` registers an `FSceneViewExtension`.
- `FSGSRViewExtension::BeginRenderViewFamily()` installs `FSGSRSpatialUpscaler` with `SetPrimarySpatialUpscalerInterface()` when temporal upscaling is not requested.
- The upscaler executes RDG subpasses (first, scaler, last) through `ISpatialUpscaler::AddPasses()`.
- Runtime enable CVar: `r.Qualcomm.SGSR.Enabled`.
- The plugin source asserts UE4.27.1+; the candidate engine source reports UE4.27.2, so the declared version range is compatible.

Important: SGSR1 itself includes adaptive sharpening. Do not automatically stack a second sharpener at full strength; expose the shared sharpness control through the plugin's supported shader parameters.

### Qualcomm SGSR2

Source branch: https://github.com/SnapdragonGameStudios/snapdragon-game-plugins-for-unreal-engine/tree/engine/4.27/Plugins/SGSR2

UE4.27 package variants include `2passNoAlpha`, `2pass_fs_taau`, and `3pass`. Qualcomm's README recommends choosing the mobile path based on actual device GPU timing. The 2-pass fragment/TAAU variant is the sensible first Android integration candidate, subject to validating the target render path.

Integration pattern observed:
- Runtime plugin modules `GSR` and `GSRTUModule`.
- `FGSRViewExtension` registers a view extension.
- `BeginRenderViewFamily()` assigns the temporal upscaler using `SetTemporalUpscalerInterface()` when temporal upscaling is requested.
- Runtime CVar: `r.SGSR2.Enabled`; quality CVar: `r.SGSR2.Quality`.
- Requires TAA, temporal upsampling, mobile Gen4 TAA support, and the project packaging/configuration documented by Qualcomm.
- Uses temporal history, depth and velocity information; missing or incompatible inputs must not be silently substituted.

### AMD FidelityFX Super Resolution 1

Official UE4 plugin guide: https://gpuopen.com/learn/ue4-fsr/

- AMD states the UE4 plugin targets UE4.27.1 or later.
- Runtime enable CVar: `r.FidelityFX.FSR.Enabled`.
- Internal render percentage is controlled using the engine screen-percentage path.
- FSR1 has integrated RCAS sharpening. When the separate AMD CAS option is enabled, disable FSR RCAS to avoid double sharpening.
- AMD documents a hybrid-upscaling rule: when temporal upsampling is requested, the temporal primary upscaler takes priority over FSR's spatial pass. Therefore SGSR2/TAAU and FSR1 must be mutually exclusive in the selection controller.

### CAS

AMD CAS is a sharpening pass, not a replacement for the selected upscaler. The FidelityFX CAS algorithm is designed to be integrated as a full-screen shader pass; AMD describes it as a small engine-side shader integration with no separate subsystem or motion-vector resources. Source guide: https://gpuopen.com/learn/integrating-fidelityfx-ego-engine/ . Implement it as a separate post-upscale pass with a 0–1 strength control. Avoid double sharpening with SGSR1 and FSR1 RCAS; default CAS OFF. An older community UE4 CAS plugin exists, but its README says it was developed/tested on UE4.24 Windows and requires renderer source modification for its custom upsample callback, so it is not a drop-in Android plugin for this game.

## Lightweight spatial methods

- Nearest and Bilinear are small RDG screen passes sampling the scene-color texture to the selected output extent. They do not need a vendor plugin.
- MWSR is not yet identified by a verifiable UE4.27 source/package in this research. Public search returns unrelated uses of the acronym, so its intended algorithm/source cannot be safely inferred. Keep it as an adapter slot; integrate its shader as a spatial RDG pass once the exact algorithm/source is identified. Do not label an unrelated filter as MWSR.
- "FSR Lite" is a project UI label, not the official name of AMD's UE4 FSR1 plugin. It must refer to a defined mobile-tuned FSR1 configuration, not a fictitious separate AMD algorithm.

## Resolution and preset policy

- Output Resolution and Render Scale are independent.
- Output Resolution choices: Auto/Native, 720p, 1080p, 1440p, 2160p; expose only modes supported by the game renderer and Android output surface, clamping/hiding unsupported modes.
- For the Galaxy M14 4G, native panel is 1080 x 2408 (portrait dimensions); landscape rendering uses the corresponding 2408 x 1080 orientation.
- Unified preset list: Ultra Performance, Performance, Balanced, Quality, Ultra Quality, Custom.
- Each upscaler maps the shared preset names to its own valid scale/parameters. Custom exposes Render Scale directly. Render Scale must never rewrite Output Resolution.

## Frame Generation and scheduling

- No verified UE4.27 + Adreno 610 drop-in Qualcomm Frame Generation plugin has been established.
- Frame Generation stays Off until a real intermediate-frame algorithm is integrated and validated.
- When FG is active, automatically enable only the pacing/scheduling coordination required by FG.
- Frame Pacing remains a separate user setting.
- Low Latency labels: Off / On / Ultra. On reduces queue depth conservatively. Ultra targets zero queued frames; it may only be enabled if the active backend can guarantee safe zero-queue operation. Do not expose the old label "No Queue".

## Why the source plugins cannot simply be copied into the shipped APK

These are UE runtime source plugins, not standalone Android APK add-ons. The source must be compiled against the matching UE engine/project build, shader permutations must be compiled/cooked for the target RHI and Android GPU, and the resulting native modules/shaders must be included in the game package. The Bright Memory research repository currently contains the APK inspection workflow, configuration, and research notes—not the original BMI game project, cooked shader build pipeline, or a matching Android UE4 build environment.

The public UE4.27.2 engine-source candidate is useful for compiling/testing the plugin integration path, but it is not proven to be the exact engine snapshot used by the game. A generic rebuilt `libUE4.so` is not a safe drop-in replacement for the shipped game library.

## Implementation status

This document records source-verified integration patterns and the settings contract. It does not claim that plugins have been compiled, linked into Bright Memory, or tested on the Galaxy M14. No modified APK has been produced.
