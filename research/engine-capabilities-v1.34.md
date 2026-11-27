# Bright Memory: Infinite v1.34 — verified engine capability notes

Source package identity:
- APK: `BrightMemory-v1.34.apk`
- Size: `2,431,306,902` bytes
- SHA-256: `c35b05ac7b19fd087844de2b6261d33fbe103a6a5714c5e42d6101fab07881e4`
- Native engine: `lib/arm64-v8a/libUE4.so`
- ELF: 64-bit AArch64 Android shared object; built with Android NDK r27c; Build ID `09ae6051d345ede937255d6bd76abefbf6d2b7e8`.

## Confirmed native settings API

The extracted engine symbol table contains the following existing Unreal Engine settings functions. Offsets are relative to the ELF image:

| Capability | Native function | Offset |
|---|---|---:|
| Overall quality | `UGameUserSettings::SetOverallScalabilityLevel(int)` | `0x87359ec` |
| Shadow quality | `UGameUserSettings::SetShadowQuality(int)` | `0x8735c98` |
| View distance | `UGameUserSettings::SetViewDistanceQuality(int)` | `0x8735c88` |
| Anti-aliasing quality | `UGameUserSettings::SetAntiAliasingQuality(int)` | `0x8735ca8` |
| Texture quality | `UGameUserSettings::SetTextureQuality(int)` | `0x8735cb8` |
| Visual effects quality | `UGameUserSettings::SetVisualEffectQuality(int)` | `0x8735cc8` |
| Foliage quality | `UGameUserSettings::SetFoliageQuality(int)` | `0x8735ce8` |
| Shading quality | `UGameUserSettings::SetShadingQuality(int)` | `0x8735cf8` |
| Post-processing quality | `UGameUserSettings::SetPostProcessingQuality(int)` | `0x8735cd8` |
| Resolution scale (integer) | `UGameUserSettings::SetResolutionScaleValue(int)` | `0x8735a9c` |
| Resolution scale (float) | `UGameUserSettings::SetResolutionScaleValueEx(float)` | `0x8735b40` |
| Normalized resolution scale | `UGameUserSettings::SetResolutionScaleNormalized(float)` | `0x8735be0` |
| Dynamic resolution toggle | `UGameUserSettings::SetDynamicResolutionEnabled(bool)` | `0x87338cc` |
| Apply settings | `UGameUserSettings::ApplySettings(bool)` | `0x8734cfc` |
| Save settings | `UGameUserSettings::SaveSettings()` | `0x873500c` |

The engine also contains the Unreal scalability implementation, including `Scalability::SetQualityLevels`, `Scalability::FQualityLevels` setters for view distance, shadows, anti-aliasing, textures, effects, foliage, post-processing and shading, and `Scalability::GetResolutionScreenPercentage`.

## Render pipeline evidence

The binary contains:
- `ISpatialUpscaler::AddDefaultUpscalePass`
- `ITemporalUpscaler::GetDefaultTemporalUpscaler`
- `FDefaultTemporalUpscaler::AddPasses`
- `GTemporalUpscaler`

This confirms Unreal's built-in spatial/temporal upscaler interfaces are present. It does **not** prove that SGSR1/SGSR2, FSR Lite, MWSR, AMD CAS, or a custom frame-generation implementation is already integrated. No verified SGSR/FSR/FidelityFX implementation symbol was found in the inspected symbol listing.

## Implementation constraints

1. Prefer calling the existing `UGameUserSettings` / Scalability APIs for real quality and render-scale controls rather than adding UI-only toggles.
2. Verify each setting's effect in-game before exposing it as functional; symbol presence alone does not prove a particular quality tier produces the intended visual result.
3. A true custom upscaler or frame-generation implementation still requires a valid render-graph/RHI integration and the required shader inputs (and motion vectors/depth for interpolation). Do not simulate frame generation by duplicating frames.
4. The current evidence is from a stripped AArch64 engine binary and package inspection, not a source build. Avoid blind binary patching at these offsets until calling conventions, relocation/base address, thread context, and runtime behavior are independently verified.
5. The APK's asset index shows game payloads under `assets/main.obb.png` and `assets/data`; these need separate container inspection before attempting any cooked-asset/config modifications.

## Current verified scope

The package hash and APK ZIP integrity were validated in the successful reassembly/inspection run. The separate free-build workflow is being repaired to use the runner's preinstalled Android SDK; it has not yet completed its MEGA restore/inspection stages after the latest workflow correction.
