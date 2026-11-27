# Bright Memory: Infinite Android v1.34 — verified rendering path

## Input integrity
- Reassembled original APK SHA-256: `c35b05ac7b19fd087844de2b6261d33fbe103a6a5714c5e42d6101fab07881e4`
- APK size: 2,431,306,902 bytes; ABI: arm64-v8a.
- `lib/arm64-v8a/libUE4.so` SHA-256: `94ee451ed42c4aee4c885aac3e6b6ccef2d3ab109e01a86b10e527c32c82b35e`
- Engine library size: 174,981,736 bytes.
- Game content is inside nested ZIP/Play Asset Delivery containers: `assets/data` contains eight `pakchunk*-Android_ASTC.pak` files; `assets/main.obb.png` is itself a ZIP containing `BMI_Mobile/Content/Paks/pakchunk0-Android_ASTC.pak`.

## Confirmed native engine symbols
- `UGameUserSettings::SetResolutionScaleValue(int)` — `0x8735a9c`
- `UGameUserSettings::SetResolutionScaleValueEx(float)` — `0x8735b40`
- `UGameUserSettings::SetResolutionScaleNormalized(float)` — `0x8735be0`
- `UGameUserSettings::SetOverallScalabilityLevel(int)` — `0x87359ec`
- `UGameUserSettings::SetDynamicResolutionEnabled(bool)` — `0x87338cc`
- `Scalability::SetQualityLevels(const FQualityLevels&, bool)` — `0x8b29780`
- `FQualityLevels::SetViewDistanceQuality(int)` — `0x8b2c680`
- `FQualityLevels::SetAntiAliasingQuality(int)` — `0x8b2c6d0`
- `FQualityLevels::SetShadowQuality(int)` — `0x8b2c720`
- `FQualityLevels::SetTextureQuality(int)` — `0x8b2c7c0`
- `FQualityLevels::SetEffectsQuality(int)` — `0x8b2c810`
- `FQualityLevels::SetFoliageQuality(int)` — `0x8b2c860`
- `FQualityLevels::SetShadingQuality(int)` — `0x8b2c8b0`
- `FQualityLevels::SetPostProcessQuality(int)` — `0x8b2c770`
- `Scalability::GetResolutionScreenPercentage()` — `0x8b281cc`
- `GetUpscaleMethod()` — `0x78fae10`
- `ISpatialUpscaler::AddDefaultUpscalePass(...)` — `0x78fae30`
- `GTemporalUpscaler` global pointer — `0xa8df7e8`

Disassembly of `GetUpscaleMethod()` shows it reads a global-backed integer and clamps the returned enum to 0..5. The enum-to-algorithm mapping is not yet proven.

## Proven vs unproven
- Proven: genuine engine APIs exist for resolution scale and graphics scalability; these are not merely labels.
- Proven: spatial-upscaler dispatch and a temporal-upscaler pointer exist.
- Not proven: SGSR1, SGSR2, MWSR, FSR Lite, AMD CAS, or any specific vendor algorithm is implemented/enabled in this exact binary.
- Not proven: a new settings screen can safely call these functions without the game-specific UObject lifecycle, UI bindings, and render-thread synchronization.
- A loose file at the APK root will not replace packaged content inside the nested asset packs/Pak files.

## Safe implementation boundary
The first functional change should bind the game's settings UI to its own `UGameUserSettings` and `Scalability::FQualityLevels` paths, then verify persistence and rendered output. Only expose upscalers after their implementations and enum mapping are proven. Do not advertise vendor algorithms as working until shader code and render-pass integration are actually present.

## Verification
- Reassembly run: https://github.com/tiromixo2013-netizen/BrightMemory-Android-Research/actions/runs/46025795349
- Short-lived original input artifact: `brightmemory-original-build-inputs`
- Engine analysis artifact: `brightmemory-v1.34-engine-analysis`

## Verified built-in spatial upscale algorithms
The game's `GetUpscaleMethod()` has the same 0..5 clamp and dispatch structure as Unreal's `r.Upscale.Quality` implementation. Epic documents the six modes as: 0 Nearest, 1 Bilinear, 2 Directional/unsharp, 3 Catmull-Rom bicubic, 4 Lanczos 3, and 5 Gaussian unsharp. Source: https://dev.epicgames.com/documentation/unreal-engine/screen-percentage-with-temporal-upscale-in-unreal-engine?lang=en-US

The game's `AddDefaultUpscalePass` disassembly reads the method enum and passes it to `FGlobalShaderMap::GetShader` as the pixel-shader permutation index. This is a real shader dispatch path, not a label-only selector. A nearby render-thread float global is clamped and written into the upscale pass's `UpscaleSoftness` parameter, consistent with Unreal's `r.Upscale.Softness` control for Gaussian unsharp mode.

This confirms built-in Nearest/Bilinear/Directional/Catmull-Rom/Lanczos/Gaussian spatial scaling in the engine binary. It does not establish SGSR, FSR, MWSR, or AMD CAS support.
