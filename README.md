# Bright Memory: Infinite — Android Graphics Research

This repository is a research and integration scaffold for Bright Memory: Infinite v1.34 on Android. It does not currently contain a verified modified APK.

## Active experimental profile

Target device: Samsung Galaxy M14 4G, Android 15, ARM64, Adreno 610. The inspected package configuration indicates Android OpenGL ES; Vulkan is disabled.

The agreed in-game menu order is:

1. Resolution Controller
2. Graphics Quality: Low / Medium / High / Very High / Ultra / Extreme
3. Render Scale
4. SGSR1
5. Upscale Sharpness
6. Unified Upscale Preset: Ultra Performance / Performance / Balanced / Quality / Ultra Quality
7. AMD CAS: On / Off
8. CAS Sharpness
9. Low Latency: Off / On / Ultra (Zero Queue)
10. Frame Generation Method
11. Frame Generation Multiplier: x2 / x3 / x4

No Frame Pacing, Performance HUD, or alternate upscaler selector is part of this profile. SGSR1 is the only requested upscaler.

## Implementation status

- Runtime settings bridge and Blueprint-callable API: source present.
- Graphics Quality: bounded UE4 scalability and optional mobile CVar overrides are implemented in source; actual visual effect remains unverified on the shipped game.
- SGSR1: the official Qualcomm UE4.27 plugin's primary spatial-upscaler path has been inspected. Patch `engine-extension/renderer/patches/0002-sgsr1-runtime-sharpness.patch` exposes the real mobile edge-sharpness shader parameter; patch applicability is checked by CI. The exact shipped plugin revision and target compilation remain unverified.
- AMD CAS: a registered renderer pass is not yet available, so the runtime rejects enabling it.
- Low Latency Off/On: maps to UE4's Game Thread lead setting. Ultra/Zero Queue remains disabled until a safe, measurable control is integrated into the active Android GLES presentation path.
- Frame Generation: selection and capability gates exist. The GLES reprojection shader is a prototype, not a complete frame-generation backend. No backend is registered; x2/x3/x4 must remain unavailable.
- Original UMG graphics widget identified as `BMI_Graphics_UMG_C` within `BMI_Menu_Seting_UMG_C`; its cooked event graph has not yet been rebound to the runtime API.
- The current GitHub workflows perform source validation and APK/source inspection. They do not compile the UE4.27.2 plugin or renderer, cook modified UMG assets, or build a modified Android APK.

## Verification boundary

Successful static source checks are not proof of UE compilation, GLES shader compilation, renderer ABI compatibility, a working in-game menu, or device performance. Do not claim a feature is working until it has passed the corresponding build and on-device A/B test.

The exact UE4.27.2 engine snapshot and Qualcomm plugin revision used by the shipped game have not been established. See `engine-extension/renderer/SGSR1-INTEGRATION.md` for the official SGSR1 source findings and sharpness patch. The public reference engine source is not proof of an identical target source tree.

## Safety and distribution

This repository contains research notes, integration source, and validation scripts. It does not distribute the copyrighted game package or extracted game assets.
