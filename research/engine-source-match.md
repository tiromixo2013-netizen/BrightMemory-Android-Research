# Engine source candidate match: Bright Memory Infinite Android

Date checked: 2026-11-26

## Finding

A public Unreal Engine source repository was found:

- Repository: https://github.com/jsuk10/UE4.27plus_Custom
- Branch: `main`
- Current checked commit: `2e6b53c74a8290ffefe72cf36ac2b051687cc544`
- Repository README describes it as a custom UE4.27-plus engine fork.
- `LICENSE.md` says Unreal Engine use is governed by the Unreal Engine EULA.

Its `Engine/Build/Build.version` reports:

- MajorVersion: 4
- MinorVersion: 27
- PatchVersion: 2
- Changelist: 0
- CompatibleChangelist: 17155196
- BranchName: `++UE4+Release-4.27`

This is a complete-looking engine source tree, not merely a game plugin or headers. It includes `Engine/Source/Runtime/Renderer`, `Engine/Source/Runtime/Engine`, and `Engine/Source/Runtime/VulkanRHI`, including Android Vulkan source files.

## Evidence from the Bright Memory Android library

Analyzed binary:

- File: `libUE4.so`
- ELF: AArch64 Android shared library
- Build ID: `09ae6051d345ede937255d6bd76abefbf6d2b7e8`

Embedded source paths in the binary include:

- `E:/UE4.27Plus/Engine/Source/Runtime/...`
- `G:/BMI_Mobile_Android 4.27/Plugins/Products/...`

The second path identifies the game project directory name used when the library was compiled. It is not present in the public engine repository checked here.

## Source-to-binary cross-checks

### Mobile upscaler dispatch

Binary symbol:

- `AddMobilePostProcessingPasses(FRDGBuilder&, FViewInfo const&, FMobilePostProcessingInputs const&, bool)`
- Address: `0x78870c0`

The disassembly around `0x7888fc4–0x7889020` contains the custom-upscaler virtual dispatch and fallback call to:

- `ISpatialUpscaler::AddDefaultUpscalePass`
- Address: `0x78fae30`

The candidate source contains the matching mobile post-processing function in:

- `Engine/Source/Runtime/Renderer/Private/PostProcess/PostProcessing.cpp`
- `AddMobilePostProcessingPasses` begins at line 1428.
- The primary-upscale section checks `View.Family->GetPrimarySpatialUpscalerInterface()`, calls `CustomUpscaler->AddPasses(...)`, and otherwise calls `ISpatialUpscaler::AddDefaultUpscalePass(...)` (the corresponding source block is around lines 2213–2242).

This is a strong structural match for the renderer path used by the binary.

### Render Scale

Candidate source:

- `Engine/Source/Runtime/Engine/Private/GameUserSettings.cpp`
- `UGameUserSettings::SetResolutionScaleValueEx` begins around line 764.
- It clamps the requested scale, stores resolution quality, and calculates internal target dimensions.

Binary:

- `UGameUserSettings::SetResolutionScaleValueEx(float)` at `0x8735b40`.
- Disassembly stores the scale and calculates scaled width/height in the expected path.
- `SetResolutionScaleNormalized(float)` is at `0x8735be0`.

The source and binary behavior are consistent.

### Frame-rate limit

Candidate source:

- `UGameUserSettings::SetFrameRateLimitCVar(float)` in `GameUserSettings.cpp` around line 319 calls `GEngine->SetMaxFPS(...)`.

Binary:

- `UGameUserSettings::SetFrameRateLimitCVar(float)` at `0x87340ec`.
- Disassembly obtains the engine interface and dispatches through the corresponding virtual function slot.

This is another consistent source-to-binary match.

### Vulkan

The binary contains symbols for:

- `FVulkanSwapChain::RenderThreadPacing`
- `FVulkanSwapChain::Present`
- `VulkanDynamicAPI::vkQueueSubmit`
- `VulkanDynamicAPI::vkQueuePresentKHR`

The candidate repository includes the Android Vulkan RHI source tree, including `VulkanAndroidPlatform.cpp`. This establishes that the relevant engine source area is available, but does not yet prove that every game-specific Vulkan modification matches.

## Confidence and limitations

**Assessment: strong UE4.27.2 engine-source candidate; exact source identity not proven.**

Reasons it is a strong candidate:

1. The game binary embeds the `E:/UE4.27Plus/Engine` source root.
2. The repository is explicitly a UE4.27-plus custom engine source tree.
3. Its Build.version identifies UE 4.27.2.
4. Important renderer and GameUserSettings function structures agree with the binary.

What remains unproven:

- The binary does not expose a source commit SHA.
- The repository's engine source was imported in a small public Git history; it does not preserve enough upstream history to prove it is the exact snapshot used by the game.
- The game project directory `BMI_Mobile_Android 4.27` and its private product plugins are not included in this engine repository.
- No modified engine, game library, or APK has been built.

## Practical consequence

This source can be used to develop and compile an UE4.27.2 engine-side implementation and to study the correct RDG / spatial-upscaler integration path. It does **not** by itself reproduce the Bright Memory game build: the game project, its plugins, configuration, cooked assets/shaders, and compatible build setup are still missing.

Do not treat a loose plugin or a rebuilt generic UE4.27.2 library as a drop-in replacement for the game's `libUE4.so`.

## Additional public evidence: official BMI mod kit version

A public Bright Memory: Infinite mod-development guide from Playism points users to the BMI_ModDevelopmentKit_V1.0 and explicitly instructs them to use Unreal Engine **4.26.2** for that kit. It describes content-only model/asset modding and PAK packaging, not rebuilding the Android renderer or the shipping Android engine.

Source: https://steamcommunity.com/sharedfiles/filedetails/?id=3053538813

This is a version discrepancy, not proof that the Android v1.34 binary was built with 4.26.2. The Android binary's embedded `UE4.27Plus` paths and the renderer/GameUserSettings structural matches still make the public UE4.27.2 fork a stronger engine-source candidate for the Android target. The mod kit is useful for understanding BMI asset layout and cooking conventions, but it is not a substitute for the Android engine source or the missing mobile game project.

## Current source-search conclusion

- **Best available engine source candidate:** `jsuk10/UE4.27plus_Custom`, commit `2e6b53c74a8290ffefe72cf36ac2b051687cc544`.
- **Public BMI mod kit:** UE4.26.2 content-mod workflow; not the matching Android engine source.
- **Exact Android source snapshot:** not found in public evidence yet. The private project/plugin source path embedded in the binary (`BMI_Mobile_Android 4.27`) is not contained in the public engine fork.
- Next validation: compare more renderer, VulkanRHI, and GameUserSettings function bodies against the Android binary before treating the fork as a build base. Do not overwrite the target engine binary until ABI/build compatibility is demonstrated.