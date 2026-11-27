# Qualcomm SGSR integration audit (pinned source)

Repository: https://github.com/SnapdragonGameStudios/snapdragon-game-plugins-for-unreal-engine
Pinned commit: `761e8393ae44a6cef48e51f37e315ccce494b055`
Submodule path: `third_party/qualcomm-ue-plugins`

## Verified source availability

The pinned repository contains both SGSR generations, not just SGSR1:

- SGSR1 plugin: `Plugins/SGSR/SGSR1.uplugin`, declares UE 4.27.0 and Android support.
- SGSR1 render integration: `Plugins/SGSR/Source/SGSRSpatialUpscaling/Private/SGSRViewExtension.cpp`; it registers an `ISpatialUpscaler` through `FSceneViewFamily::SetPrimarySpatialUpscalerInterface` when spatial upscaling is requested.
- SGSR1 shader sources: `Plugins/SGSR/Shaders/Private/PostProcessSGSR.usf` and `sgsr.ush`.
- SGSR2 has three variants: `Plugins/SGSR2/3pass`, `3passNoAlpha`, and `2passNoAlpha`.
- The `2pass_fs_taau` SGSR2 plugin declares UE 4.27.0 and Android support. Its `FGSRViewExtension::BeginRenderViewFamily` assigns the temporal upscaler only when the view requests temporal upscaling and no temporal upscaler is already installed.

## Important SGSR2 requirements / risks

The upstream SGSR2 README requires mobile TAA and temporal upsampling, with settings including:
- `r.DefaultFeature.AntiAliasing=2`
- `r.TemporalAA.Upsampling=1`
- `r.SGSR2.Enabled=1`
- `r.SGSR2.Quality=0..3`

Its documented quality modes are Ultra Quality (1.25x), Quality (1.5x), Balanced (1.7x), Performance (2.0x). These are ratios/modes documented by upstream, not a guarantee that the BMI v1.34 project has compatible motion vectors, TAA history, material velocity output, or packaging settings.

**SGSR1 and SGSR2 must be mutually exclusive at runtime.** They both compete for the primary/temporal upscale path and SGSR2 requires temporal upscaling. The UI must select one method and restore the previous engine state when switching or disabling.

## Integration policy for Bright Memory: Infinite v1.34 ARM64

1. Do not copy both plugins into the game and enable both by default.
2. Build a single method router that selects exactly one implementation: SGSR1 spatial, SGSR2 temporal, or stock Bilinear/Nearest fallback.
3. Preserve the game's original TAA / AA configuration when SGSR2 is off; save and restore any CVars changed by the plugin.
4. Before enabling SGSR2, verify that the actual BMI project uses a compatible temporal AA path and that velocity buffers/history are valid. If not, keep SGSR2 hidden/disabled.
5. Validate Android Vulkan and OpenGL ES separately; plugin platform declarations alone do not prove the game's packaged RHI supports the plugin.
6. Keep the upstream Qualcomm source pinned; do not silently track a moving branch.

## Other agreed methods

This Qualcomm plugin repository does not contain verified implementations for MWSR, FSR Lite, AMD CAS, frame generation, or a full performance overlay. These must not be represented as implemented based on this source alone. Nearest/Bilinear can use the engine's supported spatial upscale methods once the BMI render path is integrated. Frame generation requires a real motion/interpolation pipeline and is not implemented by duplicating frames.

## Current status

- [x] Qualcomm SGSR1 and SGSR2 source locations and UE4.27 plugin metadata verified.
- [x] SGSR1 spatial and SGSR2 temporal registration paths inspected.
- [ ] BMI project/plugin integration compiled.
- [ ] BMI-specific UI wiring and settings persistence verified.
- [ ] Android ARM64 package built and installed.
- [ ] Visual correctness/performance measured on target device.

This audit is source research, not a claim of a working modified APK.
