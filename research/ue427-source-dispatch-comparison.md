# UE4.27.2 source comparison — renderer dispatch follow-up

Date: 2026-11-27

Candidate source:
- Repository: https://github.com/jsuk10/UE4.27plus_Custom
- Commit: `2e6b53c74a8290ffefe72cf36ac2b051687cc544`
- Build.version: UE 4.27.2 (previously inspected; exact BMI source snapshot remains unproven)

## Confirmed source-side behavior

### Mobile primary spatial-upscaler dispatch
File: `Engine/Source/Runtime/Renderer/Private/PostProcess/PostProcessing.cpp`

In the mobile post-processing path:
- Primary upscale is enabled when the view uses spatial screen percentage and the view rectangles differ, subject to the mobile tonemapper-upscale setting; Panini can also enable the pass.
- At the primary-upscale stage, the engine asks `View.Family->GetPrimarySpatialUpscalerInterface()`.
- If non-null, it calls the custom interface's `AddPasses(GraphBuilder, View, PassInputs)`.
- If null, it falls back to `ISpatialUpscaler::AddDefaultUpscalePass(..., EUpscaleMethod::Bilinear, ...)`.

This is the source-level extension point for a real custom spatial upscaler in this UE4.27.2 candidate. It is a stronger match to the target binary's observed custom-upscaler-vs-default dispatch than a speculative raw-offset patch, but the target binary's object provenance and exact ABI mapping are still not proven.

Relevant source lines at candidate commit:
- Mobile pass enablement around lines 1545–1558.
- Primary custom interface dispatch around lines 2211–2241.

### Resolution scale
File: `Engine/Source/Runtime/Engine/Private/GameUserSettings.cpp`
- `SetResolutionScaleValueEx(float)` clamps `ScalabilityQuality.ResolutionQuality` and updates desired screen dimensions.
- `SetResolutionScaleNormalized(float)` remaps normalized input and delegates to the value setter.
- This matches the general setter family and behavior previously identified in the target binary, but does not prove byte-for-byte source identity.

### Frame-rate CVar
Same file:
- `SetFrameRateLimitCVar(float)` delegates to `GEngine->SetMaxFPS(max(InLimit, 0))`.
- `ApplySettings` applies the effective frame-rate limit and scalability quality levels.

## What this does and does not establish

Established:
1. Candidate source has the expected UE4.27.2 renderer architecture and mobile primary spatial-upscaler extension point.
2. Candidate source contains the expected resolution-scale and frame-rate settings implementation families.
3. The custom-upscaler path is a proper render-graph pass interface, not a texture-copy placeholder.

Not established:
1. That this is the exact source snapshot used to build Bright Memory: Infinite Android v1.34.
2. That the target binary's observed view-state pointer at offset `+0xc0` is exactly the object returned by `GetPrimarySpatialUpscalerInterface()`.
3. The target's complete build configuration, private BMI plugins, Android toolchain, or ability to rebuild the original APK.
4. Runtime support for SGSR1/SGSR2/MWSR/FSR Lite or frame generation. Those require separate implementations and validation; no such support is implied by the source match.

## Safe next engineering step

Use this source only as a reference/base candidate while verifying the exact target ABI:
- trace the writer of the target's `FSceneViewInitOptions +0xb0` view-state pointer;
- identify the concrete object and vtable called by the target mobile post-processing path;
- compare additional function bodies and Android Vulkan build configuration;
- only then choose between a compatible native extension, a carefully verified binary hook, or a source-based engine rebuild.

Do not write to the discovered offsets or install a custom vtable until the object layout and lifetime are proven.
