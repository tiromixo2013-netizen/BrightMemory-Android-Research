# Pre-merge readiness review — Bright Memory: Infinite Android v1.34

Date: 2026-11-27  
Branch: `native-integration`  
Target APK SHA-256: `c35b05ac7b19fd087844de2b6261d33fbe103a6a5714c5e42d6101fab07881e4`  
Target native library: `lib/arm64-v8a/libUE4.so`  
ELF Build ID: `09ae6051d345ede937255d6bd76abefbf6d2b7e8`

This is a consolidated review of the earlier APK, asset, renderer, engine-source, Qualcomm-plugin, and run 8/9 investigations. It is a readiness report, not a claim that a patch or rebuilt APK exists.

## 1. What is actually in hand

- Original APK was reassembled from the 10 split artifacts and verified against the known 2,431,306,902-byte size and SHA-256.
- ARM64 Android `libUE4.so` is 174,981,736 bytes, AArch64 ELF shared object, with 10 program headers and a dynamic section.
- Dynamic dependencies include Android system libraries, `libc++_shared.so`, and `libplaycore.so`; the library has an existing `DT_INIT_ARRAY` (7152 bytes).
- Game content is packed under `assets/main.obb.png` and `assets/data`; nine PAKs were identified, with readable V11 indexes. The APK does not have ordinary top-level PAK/OBB files.
- The game contains cooked UMG graphics menu assets and existing commands for render scale, shadows, mobile AO and FPS limits (see `graphics-feature-integration-map-v1.34.md`).
- The public research repository does not contain the BMI project source, private BMI plugins, target device profiles, or a verified full Android rebuild pipeline.

## 2. Renderer dataflow — strongest current evidence

### Dispatch consumer

`AddMobilePostProcessingPasses` at RVA `0x78870c0` performs the mobile custom-upscaler dispatch. The observed sequence is:

1. Read the first pointer-sized field from the `FViewInfo` / `FSceneView` base object.
2. Read the candidate receiver from offset `+0xc0` of that pointed-to object.
3. If non-null, load its vtable and call slot `+0x20`.
4. If null, call the stock `ISpatialUpscaler::AddDefaultUpscalePass` path.

The virtual call receives the render-graph builder, view, mobile post-process input data, and output storage. This is a real render-graph integration point, not a UI-only toggle.

### Upstream pointer source

Earlier run 8 disassembly, recorded in `upsaler-dispatch-followup.md`, traced the first field:

- `FSceneView::FSceneView(FSceneViewInitOptions const&)` at `0x8b35598) copies `FSceneViewInitOptions +0xb0` into the first pointer-sized field of the constructed view.
- `ULocalPlayer::CalcSceneViewInitOptions` at `0x884a708` allocates an `FSceneViewStateReference`, stores the reference entry at init-options `+0xb8`, and writes a separate PlayerController virtual-call result at `+0xc0`.
- That function's inspected range did not establish the writer of init-options `+0xb0`. Do not confuse the `+0xb8` and `+0xc0` fields with `+0xb0`.

### Run 9 lifecycle result

Run 9 disassembled `FViewInfo::Init`, both `FViewInfo` constructors, `FSceneViewState` construction/destruction, `FSceneViewState::Destroy`, and `FSceneViewStateReference` allocation/destruction.

It proves that a scene-view-state allocation route exists, but does not prove that the receiver loaded from the object at `+0xc0` is the `FSceneViewState`, nor that the scene-view-state reference is the source of the `FSceneViewInitOptions +0xb0` pointer. No safe receiver-field write site has been identified yet.

## 3. Engine source provenance

Strongest public candidate:
- `jsuk10/UE4.27plus_Custom`
- Commit `2e6b53c74a8290ffefe72cf36ac2b051687cc544`
- `Build.version`: UE 4.27.2, changelist 0, compatible changelist 17155196.

The Android library contains embedded paths including `E:/UE4.27Plus/Engine/Source/Runtime/...` and `G:/BMI_Mobile_Android 4.27/Plugins/Products/...`. Renderer and GameUserSettings function structures match the public UE4.27.2 candidate in important areas. This is strong family/source-layout evidence, not proof of the exact commit used by the game.

The public Bright Memory mod kit documentation points to UE4.26.2 for a content-only PC mod workflow. That does not override the Android binary's embedded UE4.27Plus paths and does not establish the Android engine version.

## 4. Existing feature integration evidence

Confirmed native settings API families include:
- Overall and per-category scalability setters (view distance, shadows, AA, textures, effects, foliage, shading, post-processing).
- Resolution scale setters (integer, float and normalized).
- Dynamic resolution, frame-rate limit, ApplySettings and SaveSettings.

The cooked BMI menu contains actual CVar command strings for:
- `r.MobileContentScaleFactor` presets.
- `r.ShadowQuality` 0/1.
- `r.Mobile.AmbientOcclusionQuality` 0/1.
- `t.maxfps` 45/60/90/120.

These give us a practical early integration route for genuine quality/render-scale settings, subject to runtime visual validation.

## 5. Qualcomm and other upscalers

The pinned Qualcomm UE plugin submodule is:
- Repository `SnapdragonGameStudios/snapdragon-game-plugins-for-unreal-engine`
- Commit `761e8393ae44a6cef48e51f37e315ccce494b055`
- UE4.27 branch.

It contains SGSR1's `ISpatialUpscaler::AddPasses` render-graph implementation and scene-view-extension registration pattern. SGSR2 UE4.27 variants also exist, but temporal modes require compatible TAA/history/motion-vector configuration. Only one primary spatial upscaler should be selected at a time.

No verified target implementation has been found for MWSR, the exact requested FSR Lite variant, AMD CAS, or true x2/x3/x4 frame generation. These remain separate implementation tasks. SGSR2 must not be relabeled as spatial SGSR1; frame generation must not be simulated by repeating frames.

## 6. Integration strategy and gates

Preferred source-level architecture, if a compatible build route becomes available:
1. Use the UE4.27 view-extension lifecycle.
2. Register one dispatcher/selection policy.
3. Set one primary upscaler for the view family.
4. Implement the selected algorithm's RDG pass, resource lifetime and `Fork_GameThread` semantics.
5. Preserve the stock fallback and restore original AA/screen-percentage settings when disabling or switching.
6. Reuse existing game settings and cooked menu assets only after a compatible UAsset editing or runtime UI route is proven.

For the user's required existing-binary approach, do not blindly replace the receiver pointer, vtable, or machine code. First prove the `+0xb0` writer, the concrete object at `+0xc0`, its lifetime/thread ownership, the exact virtual signature, and a safe executable-code/data insertion strategy for this ELF. The ELF's existing init array and dynamic dependencies are observable facts, not permission to modify them without a loader/ABI test.

## 7. Current automated follow-up

- Run 10 scans the complete disassembly for `+0xc0` / `+0xc8` memory-operation candidates.
- Run 11 additionally disassembles `FSceneView::FSceneView(FSceneViewInitOptions const&)`, `FSceneView` copy/move constructors, `ULocalPlayer::CalcSceneViewInitOptions`, and `FSceneViewFamily` construction.
- These runs must be inspected before selecting a patch point.

## 8. Go/no-go

**Ready for:** a targeted native integration prototype after the receiver producer and lifetime are proven; initial non-upscaler settings can be prototyped against existing engine APIs/CVars.

**Not ready for:** claiming a finished custom-upscaler injection, enabling SGSR2 without temporal validation, exposing MWSR/FSR Lite/CAS/FG as working, or producing a trustworthy modified APK.

No engine bytes were patched and no modified APK was built by the run 8/9 audits.