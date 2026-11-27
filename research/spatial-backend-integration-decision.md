# Spatial backend integration decision — Bright Memory: Infinite v1.34

Target: UE4.27-era Android GLES renderer on Galaxy M14 4G (Adreno 610).

## Required method set

The menu exposes exactly one FSR entry: **FSR Mobile Lite**. Do not add a second generic FSR1/EASU/RCAS entry, and do not wire the full desktop-style multi-pass FSR path.

The selected FSR implementation is atyuwen's optimized mobile FSR1 (vendor/FSR-Mobile/opt_fsr.fxh), which fuses EASU reconstruction and RCAS-style contrast limiting/sharpening in one pixel-shader function. Its implementation uses six FsrEasuTapH calls for the directional reconstruction path and a compact neighborhood path. It expects the host shader to provide FsrEasuSampleH, EASU constants, and AMD portability macros from ffx_a.h / ffx_fsr1.h. Integrate it as one UE4.27 RDG screen pass; do not add a separate RCAS pass.

## MWSR

Use the existing MIT-licensed upstream shader in vendor/MWSR/mwsr-up.hlsl, sourced from linckosz/moonlight-web. This is a real algorithm source, not an already-integrated UE shader. Before registering method 2, port its host interface and constants to UE4.27 global-shader/RDG conventions, replace unsupported derivative assumptions if required by the target GLES path, and compile the shader for the game's actual RHI. Do not expose the method until the RDG pass returns a valid output on the target renderer.

## SGSR1

The vendored Qualcomm SGSR1 source includes its shader and first/scaler/last subpasses. It is not a single callback that can safely be dropped into the current unified pipeline. Its current FSGSRSpatialUpscaler owns per-view FSGSRData, runs setup/resource creation and the scaler subpass, and expects /Plugin/SGSR shader directory mapping. Port those subpasses into an adapter owned by the BMI pipeline, and ensure the vendor FSGSRViewExtension does not compete for primary or secondary spatial upscaler selection.

## SGSR2

No SGSR2 implementation is currently present in the vendored source tree. Keep it unavailable until a specific source, license, shader model and UE4.27 mobile integration are verified. Do not alias SGSR2 to SGSR1.

## Unified pipeline contract

One primary spatial reconstruction interface owns the order:
1. selected reconstruction (SGSR1 / SGSR2 when genuinely ported / MWSR / FSR Mobile Lite / Nearest / Bilinear)
2. optional AMD CAS pass
3. optional Performance HUD composition

Frame Generation remains a separate temporal/presentation system. Frame pacing and low-latency queue controls remain separate frame-lifecycle controls, not pixel reconstruction shaders.

## Validation gates before enabling a method

- UE4.27 shader compilation for the actual game RHI (the shipped profile disables Vulkan; do not assume Vulkan).
- Valid input/output texture extents, formats, view rects and RDG resource lifetime.
- No competing primary/secondary upscaler extension.
- Device test on Adreno 610 for visual artifacts, GPU cost and frame time.
- Keep unavailable methods hidden and report N/A rather than claiming support.

## Provenance

- FSR Mobile Lite: https://gist.github.com/atyuwen/78d6e810e6d0f7fd4aa6207d416f2eeb (MIT mobile optimization; AMD FSR1 helper headers retain AMD MIT notices).
- MWSR: https://github.com/linckosz/moonlight-web/tree/main/mwsr (MIT).
- SGSR1: Qualcomm Innovation Center source, BSD-3-Clause; preserve upstream notices and disclaimer.
- CAS: AMD FidelityFX CAS source, MIT; preserve upstream notice.
