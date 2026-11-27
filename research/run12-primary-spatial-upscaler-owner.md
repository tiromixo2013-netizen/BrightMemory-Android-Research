# Run 12 interpretation: exact spatial-upscaler receiver identified

Date: 2026-11-27  
Target binary: Bright Memory: Infinite Android v1.34, ARM64, Build ID `09ae6051d345ede937255d6bd76abefbf6d2b7e8`.

## Critical correction

The receiver at offset `+0xc0` is **not a member of FSceneViewState**. The first pointer in `FSceneView` is `Family`, not `State`. The dataflow resolves to the standard `FSceneViewFamily::PrimarySpatialUpscalerInterface` field.

This is now supported by matching target disassembly and the UE4.27.2 candidate source layout.

## Target-binary proof

### FSceneViewInitOptions -> FSceneView

In target `FSceneView::FSceneView(FSceneViewInitOptions const&)`, RVA `0x8b35598`:

- `0x8b355e8: ldr x8, [x1, #0xb0]`
- `0x8b355f4: str x8, [x0]`
- `0x8b355f8: ldr x8, [x1, #0xb8]`
- `0x8b35614: str x8, [x0, #0x8]`

The UE4.27.2 `FSceneView` declaration starts with:
- `const FSceneViewFamily* Family;`
- `FSceneViewStateInterface* State;`

The target's `ULocalPlayer::CalcSceneViewInitOptions` at `0x884a708` allocates a `FSceneViewStateReference`, loads its `Reference` pointer, and stores that pointer at `FSceneViewInitOptions +0xb8`. Thus the first pointer at options `+0xb0` is the family pointer, while `+0xb8` is the state pointer.

### AddMobilePostProcessingPasses receiver

At target RVA `0x7888fb4` onward:
- load the first pointer from the `FViewInfo` / `FSceneView` base (the `Family` pointer);
- load `[Family + 0xc0]`;
- if non-null, invoke its vtable slot `+0x20`;
- otherwise call `ISpatialUpscaler::AddDefaultUpscalePass`.

### FSceneViewFamily field layout

The UE4.27.2 candidate `FSceneViewFamily` declares these consecutive private interface pointers:
1. `ISceneViewFamilyScreenPercentage* ScreenPercentageInterface`
2. `const ITemporalUpscaler* TemporalUpscalerInterface`
3. `ISpatialUpscaler* PrimarySpatialUpscalerInterface`
4. `ISpatialUpscaler* SecondarySpatialUpscalerInterface`

This places `PrimarySpatialUpscalerInterface` at `+0xc0` in the candidate layout. The target's `FSceneViewFamily` constructor at RVA `0x8b3b840` performs `stp q0, q0, [x19, #0xb0]` at `0x8b3b9a0`, zeroing the four consecutive 64-bit interface pointers at offsets `+0xb0`, `+0xb8`, `+0xc0`, and `+0xc8`. This independently corroborates the field layout in the target binary.

## Exact API and ABI match

The UE4.27.2 candidate `PostProcessUpscale.h` declares:

`FScreenPassTexture ISpatialUpscaler::AddPasses(FRDGBuilder&, const FViewInfo&, const ISpatialUpscaler::FInputs&) const`

The target call-site register setup matches:
- `x0`: `ISpatialUpscaler*` receiver from `Family +0xc0`;
- `x1`: `FRDGBuilder&`;
- `x2`: `FViewInfo const&`;
- `x3`: `ISpatialUpscaler::FInputs const&`;
- `x8`: indirect return storage for `FScreenPassTexture`.

This makes the receiver's semantic type and intended virtual method strongly identified. The precise target vtable slot still merits verification from target vtable/relocation evidence, but the signature and calling convention match.

## Intended producer and lifecycle

The standard engine API is `FSceneViewFamily::SetPrimarySpatialUpscalerInterface(ISpatialUpscaler*)`. The standard place to call it is `ISceneViewExtension::BeginRenderViewFamily(FSceneViewFamily&)`.

Qualcomm's pinned SGSR1 plugin implements exactly this pattern:
- `FSGSRModule::StartupModule()` calls `FSceneViewExtensions::NewExtension<FSGSRViewExtension>()` and retains the thread-safe shared pointer.
- The plugin descriptor loads the `SGSR` runtime module at `PostEngineInit` and the `SGSRSpatialUpscaling` runtime module at `PostConfigInit`.
- `FSGSRViewExtension::BeginRenderViewFamily` checks whether temporal upscaling is requested;
- for spatial mode, it calls `InViewFamily.SetPrimarySpatialUpscalerInterface(new FSGSRSpatialUpscaler(...))`;
- `FSGSRSpatialUpscaler::AddPasses` implements the RDG upscaling pass;
- `Fork_GameThread` creates the per-render-family copy.

Sources at Qualcomm commit `761e8393ae44a6cef48e51f37e315ccce494b055`:
- `Plugins/SGSR/Source/SGSRSpatialUpscaling/Private/SGSRViewExtension.cpp`
- `Plugins/SGSR/Source/SGSRSpatialUpscaling/Private/SGSRSpatialUpscaler.cpp`
- `Plugins/SGSR/Source/SGSRSpatialUpscaling/Private/SGSRSpatialUpscaler.h`
- `Plugins/SGSR/Source/SGSRSpatialUpscaling/Public/SGSRViewExtension.h`

The UE4.27.2 candidate `FRendererModule::BeginRenderingViewFamily` runs active view extensions' `BeginRenderViewFamily` callbacks before constructing the scene renderer. The target binary exports `FRendererModule::BeginRenderingViewFamily` at RVA `0x79e8664`, size 4792, and exports `FSceneViewExtensions::ForEachActiveViewExtension` at `0x8b3c1bc`. Run 13 should disassemble the target lifecycle function to confirm the same ordering in this exact binary.

## Practical integration consequence

We no longer need to write into a guessed `FSceneViewState` field or patch the consumer's vtable. The intended extension point is the normal view-family API:
1. load/register a compatible `ISceneViewExtension`;
2. in `BeginRenderViewFamily`, set exactly one `PrimarySpatialUpscalerInterface`;
3. implement `AddPasses` and `Fork_GameThread` with the exact target ABI and RDG lifetime rules;
4. preserve the default-upscale fallback and ensure temporal-AA requests do not incorrectly select a spatial-only implementation.

This resolves the receiver owner and normal producer mechanism. It does **not** yet solve how to load newly compiled plugin code into this already-built Android game without rebuilding UE4. That is the remaining packaging/injection gate. The public UE4.27.2 source is a close reference, not a proven exact source revision for the BMI binary.

## Run 12 scan status

Run 12 (`46003353573`) includes the corrected whole-binary offset scan and additional `FSceneViewInitOptions`/renderer lifecycle functions. Its output should still be inspected for unexpected writes and exact target callback ordering.

No engine bytes have been patched and no modified APK has been built.