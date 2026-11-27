# Bright Memory: Infinite v1.34 — verified asset-container inventory

The APK was reconstructed from six independently checksum-verified parts. Its SHA-256 is:
`c35b05ac7b19fd087844de2b6261d33fbe103a6a5714c5e42d6101fab07881e4`

## Confirmed container layout

The APK has no top-level files ending in `.pak` or `.obb`. Its Unreal asset payloads are stored inside ZIP containers with nonstandard names:

- `assets/main.obb.png` — ZIP container, 1,000,268,644 bytes, 39 entries.
- `assets/data` — ZIP container, 1,371,499,718 bytes, 130 entries.

The two containers hold nine Unreal Engine PAK files, all using the Android ASTC variant:

| PAK chunk | Size (bytes) | Container |
|---|---:|---|
| pakchunk0-Android_ASTC.pak | 914,825,939 | assets/main.obb.png |
| pakchunk1-Android_ASTC.pak | 79,921,799 | assets/data |
| pakchunk2-Android_ASTC.pak | 363,428,917 | assets/data |
| pakchunk3-Android_ASTC.pak | 417,131,038 | assets/data |
| pakchunk4-Android_ASTC.pak | 250,051,773 | assets/data |
| pakchunk5-Android_ASTC.pak | 259,576,552 | assets/data |
| pakchunk6-Android_ASTC.pak | 29,742,173 | assets/data |
| pakchunk7-Android_ASTC.pak | 2,829,649 | assets/data |
| pakchunk8-Android_ASTC.pak | 50,629,105 | assets/data |

Total PAK payload: **2,368,136,945 bytes** (about 2.37 GB).

## Confirmed engine observations

- The native engine is `lib/arm64-v8a/libUE4.so`, an AArch64 Android shared object.
- The engine exposes native `UGameUserSettings` functions for overall scalability, view distance, shadows, anti-aliasing, textures, visual effects, foliage, shading, post-processing, frame-rate limit, VSync, and render scale.
- It exposes Unreal spatial/temporal upscaler interfaces, including `ISpatialUpscaler::AddDefaultUpscalePass`, `ITemporalUpscaler::GetDefaultTemporalUpscaler`, and `FDefaultTemporalUpscaler::AddPasses`.
- Symbol presence is not proof that a particular game menu or quality preset is wired to those functions.
- No verified SGSR/FSR/FidelityFX implementation or frame-generation implementation has been identified in the inspected dynamic symbol table.

## Next safe reverse-engineering step

Parse the nine PAK headers and enumerate their internal file indexes before attempting any asset/config changes. Do not repack or patch the game until PAK encryption/compression state, mount points, and file layout are known. Native render hooks/upscalers require a separately validated engine integration; do not claim them as implemented based on symbol names alone.

## Reproducible inspection artifact

The successful GitHub Actions run `45917246738` produced artifact `brightmemory-v1.34-nested-asset-inventory`, containing the exact nested container entry lists and JSON report.
