# Unified graphics integration batch — resolution through AMD CAS

Target: Bright Memory: Infinite Android v1.34 ARM64  
Branch: native-integration  
Source APK SHA-256: `c35b05ac7b19fd087844de2b6261d33fbe103a6a5714c5e42d6101fab07881e4`

This batch consolidates the first seven agreed graphics stages into one ordered contract. It does not silently mark renderer code or cooked UMG controls as complete.

## Confirmed integration surfaces in the shipped game

### Existing in-game graphics menu assets

The extracted PAK contains the real graphics settings UI assets:
- `BMI_Graphics_UMG.uasset/.uexp`
- `BMI_Graphics_Screen.uasset/.uexp`
- `BMI_Graphics_Shadows.uasset/.uexp`
- `BMI_Graphics_SSAO.uasset/.uexp`
- `BMI_Graphics_FPSLimb_Android.uasset/.uexp`

The Screen widget contains existing `r.MobileContentScaleFactor` commands with values 0, 0.75, 0.85, 1.0, 1.2, 1.35 and 1.5, plus resolution labels. The Shadows widget contains `r.ShadowQuality` commands and the SSAO widget contains `r.Mobile.AmbientOcclusionQuality` commands. These are genuine integration anchors, not newly invented UI.

### Native engine settings APIs

The stripped ARM64 `libUE4.so` contains UGameUserSettings setters for overall scalability, view distance, shadows, anti-aliasing, textures, visual effects, foliage, shading, post-processing and resolution scale. It also contains ApplySettings and SaveSettings. The exact RVAs are recorded in `research/engine-capabilities-v1.34.md` and `config/native-graphics-integration.json`.

The game already has a mobile post-processing dispatch point at RVA `0x78870c0`, a default upscale fallback at `0x78fae30`, and a family-owned primary spatial upscaler pointer at offset `+0xc0`. Run 17 confirmed family copy/destructor behavior. This is the real renderer extension seam; do not replace it with a screen-space overlay or CVar-only upscaler claim.

## Final agreed order and implementation contract

1. **Resolution Controller** — Native, 540P, 640P, 720P, 900P, 980P, 1080P. Reuse the original Screen widget behavior, query physical display metrics, preserve aspect/orientation, and never change Android system DPI. Exact label-to-output mapping remains to be measured.
2. **Graphics Quality** — Low, Medium, High, Very High, Ultra, Extreme. Drive all available scalability categories: view distance, shadows, anti-aliasing, textures, visual effects, foliage, shading and post-processing. The engine's normal scalability levels are not six distinct quality tiers; Ultra/Extreme require verified additional mobile settings and must not be fake duplicate labels.
3. **Render Scale** — separate from display resolution, using the native UGameUserSettings resolution-scale path and applying changes at safe frame boundaries. Do not reallocate render targets every frame. Dynamic resolution stays off unless independently implemented.
4. **Upscaling Method** — one method active at a time: SGSR1, SGSR2, MWSR, FSR Lite, Nearest, Bilinear. SGSR1/2 source architectures are researched; MWSR/FSR Lite need pinned, exact implementations; Nearest/Bilinear need actual RDG sampling passes. Unreal default upscale remains fallback.
5. **Upscale Sharpness** — one shared 0–1 setting routed only to the selected method when supported.
6. **Unified Upscale Preset** — Ultra Performance, Performance, Balanced, Quality, Ultra Quality. One control; each method maps it to its own validated input scale and settings. Never stack two upscalers.
7. **AMD CAS + CAS Sharpness** — independent post-upscale sharpen-only pass, default off, sharpness 0–1. Do not use CAS as a second resizer and avoid double sharpening if the chosen upscaler already sharpens.

## AMD CAS work completed in this batch

- Official AMD FidelityFX CAS source is pinned and vendored under `vendor/amd-fidelityfx-cas/`.
- A GLES 3.1 FP32 sharpen-only adaptation is present at `CAS_Shader_GLES31.comp`.
- The portable shader workflow compiled the GLES adaptation, official HLSL and official Vulkan GLSL and validated their generated SPIR-V.
- Successful reference validation: [run 46008810503](https://github.com/tiromixo2013-netizen/BrightMemory-Android-Research/actions/runs/46008810503), artifact ID `15202620952`.

This is shader-source validation only. It is not UE4's cooked Android shader format, does not prove the shader can be loaded by BMI, and does not prove runtime execution or visual quality on Adreno 610.

## Consolidated machine-readable contract and checks

- Contract: `config/native-graphics-integration.json`
- Existing user-facing menu schema: `config/graphics-settings.json`
- Static contract validator: `tools/validate_graphics_contract.py`
- CI workflow: `.github/workflows/validate-graphics-contract.yml`

The contract deliberately keeps all new controls hidden until their actual runtime paths are implemented and validated.

## Exact delivery state — no false completion claims

| Item | State |
|---|---|
| Existing resolution menu/CVar path identified | Confirmed |
| Native render-scale API identified | Confirmed |
| Native scalability setters identified | Confirmed |
| Spatial upscaler dispatch/lifetime audited | Confirmed |
| SGSR1/SGSR2 upstream architecture inspected | Confirmed |
| Nearest/Bilinear RDG passes | Not implemented |
| MWSR/FSR Lite exact source implementations | Not pinned/implemented |
| Per-method preset/Sharpness runtime binding | Not implemented |
| AMD CAS GLES reference shader compile | Passed |
| AMD CAS UE4 shader/RDG integration | Not implemented |
| Original cooked UMG menu modified | Not yet |
| Original `libUE4.so` modified | Not yet |
| Repacked/signature-verified modified APK | Not yet |

A correct final APK requires native UE4 renderer integration against the exact stripped engine ABI and its Android shader pipeline, plus safe cooked-UMG editing or a validated runtime UI route. Changing only the JSON contract, project INI, or CVar strings would not implement the requested in-game controls or upscalers.
