# Native UE4 binary audit — run 8 findings

Source artifact: GitHub Actions run 45991989744, artifact `brightmemory-native-hook-audit-full`.
Original APK reassembly and SHA-256 verification passed. Extracted library is AArch64 Android ELF, Build ID `09ae6051d345ede937255d6bd76abefbf6d2b7e8`; library SHA-256 is recorded in the artifact.

## Confirmed binary facts

- `AddMobilePostProcessingPasses`: VA `0x78870c0`, symbol size 9616 bytes.
- `ISpatialUpscaler::AddDefaultUpscalePass`: VA `0x78fae30`, size 1424 bytes.
- `FSceneViewExtensions::ForEachActiveViewExtension`: VA `0x8b3c1bc`, size 400 bytes.
- `FDefaultTemporalUpscaler::AddPasses`: VA `0x7956ac0`, size 1184 bytes.
- `FSceneRenderer::ApplyResolutionFraction`: VA `0x79e5ba0`, size 36 bytes.
- `UGameUserSettings::SetResolutionScaleValue(int)`: VA `0x8735a9c`, size 164 bytes.
- `UGameUserSettings::SetOverallScalabilityLevel(int)`: VA `0x87359ec`, size 8 bytes; tail-branches to `Scalability::FQualityLevels::SetFromSingleQualityLevel`.
- Vulkan pacing symbols: `FVulkanSwapChain::RenderThreadPacing` at `0x626acb8`; `FVulkanSwapChain::Present` at `0x626aea8`; Android frame pacer getter at `0x67dac90`.

## Spatial-upscaler dispatch evidence

Within `AddMobilePostProcessingPasses`, the code reaches a mobile upscale stage at approximately `0x7888f70–0x7889030`. At `0x7888fc4` it loads a pointer from `[x10, #0xc0]`; if non-null it loads the virtual function pointer at vtable offset `+0x20` and calls it indirectly at `0x7888ff8`. If null, it calls `ISpatialUpscaler::AddDefaultUpscalePass` at `0x78fae30` (`0x7889020`). This proves a custom spatial-upscaler dispatch/fallback branch exists in the binary. The exact owning object, field semantics, and safe writer/registration path are **not yet proven**; do not patch the `+0xc0` location based on this observation alone.

The extension iterator at `0x8b3c1bc` walks weak `ISceneViewExtension` references and invokes a virtual callback at vtable offset `+0x98`. This confirms the engine's scene-view-extension mechanism is present, but does not by itself prove that an extension can safely install the custom upscaler on this game's build.

## Resolution and pacing

`SetResolutionScaleValue(int)` clamps the supplied scale against a stored maximum and computes internal width/height from the current desktop or configured resolution, then stores those dimensions. This is a real engine resolution-scale path, not merely a UI label.

`RenderThreadPacing` and Vulkan `Present` both contain clock-based wait/pacing logic and consult sync/frame-pacer state. Do not force a zero-depth queue or bypass waits without tracing the associated Vulkan submission, fence, and semaphore ownership.

## Temporal upscaling caution

The binary contains `FDefaultTemporalUpscaler::AddPasses`, but its presence is not evidence that SGSR2 or any external temporal algorithm is integrated or compatible. Temporal inputs, velocity, history lifecycle, and supported mobile shader platform still require verification.

## Next reverse-engineering tasks

1. Trace all reads/writes and xrefs around the `[object/vtable + 0xc0]` dispatch operand and identify the exact concrete object/type.
2. Locate the constructor/initialization writer for the custom-upscaler pointer and confirm lifetime/thread ownership.
3. Map the full call path from active view extensions to any primary-upscaler assignment API (if present in this exact binary).
4. Validate render-graph input/output structures and texture format/extent before implementing a single spatial pass.
5. Only after the dispatch and ownership proof, prepare a minimal reversible native patch. No engine bytes or game assets were modified in this audit.

## Status

Evidence extraction succeeded. This is still reverse engineering, not a modified game build. No upscaler, frame generation, graphics menu, or overlay has been implemented or runtime-tested.
