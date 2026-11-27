# Runs 10–11: scene-view state correlation and spatial-upscaler ABI

Date: 2026-11-27  
Target: Bright Memory: Infinite Android v1.34, ARM64, original `libUE4.so` Build ID `09ae6051d345ede937255d6bd76abefbf6d2b7e8`.

## Evidence captured

- Run 10: workflow `46002239668`, artifact `15198144465`.
- Run 11: workflow `46002454836`, artifact `15199395842`.
- Both reassembled APKs passed the original APK SHA-256 verification and both targeted disassembly steps completed successfully.
- Run 10's two offset-scan output files were empty. The grep expression in that workflow was incorrectly escaped, so **run 10 does not provide a valid whole-binary xref result**. A corrected scan is scheduled in run 12.

## 1. What the target binary proves

### SceneViewInitOptions to FSceneView

In `FSceneView::FSceneView(FSceneViewInitOptions const&)` at RVA `0x8b35598`:

- `0x8b355e8: ldr x8, [x1, #0xb0]`
- `0x8b355f4: str x8, [x0]`

Therefore the first pointer-sized field of the constructed `FSceneView` is copied from `FSceneViewInitOptions + 0xb0`.

The renderer consumer `AddMobilePostProcessingPasses` at `0x78870c0`:
- reads the first pointer-sized field from `FViewInfo` (the `FSceneView` base);
- reads `[that_pointer + 0xc0]`;
- if non-null, performs a virtual call at vtable byte offset `+0x20`;
- if null, invokes `ISpatialUpscaler::AddDefaultUpscalePass`.

Combined with the UE4.27 header declaration of `FSceneViewInitOptions::SceneViewStateInterface` immediately after `ViewFamily`, the `+0xb0` pointer is consistent with the scene-view-state interface pointer in this engine layout. The target's exact full class layout still needs to be validated against its own fork.

## 2. ULocalPlayer's other fields are distinct

`ULocalPlayer::CalcSceneViewInitOptions` at `0x884a708` was disassembled through its full 780-byte symbol range.

It:
- allocates a `FSceneViewStateReference` via `0x8b348f8`;
- stores the reference pointer at `FSceneViewInitOptions +0xb8`;
- obtains a PlayerController-related virtual-call result and stores it at `+0xc0`;
- does **not** write the `+0xb0` field in the inspected function.

This corrects a potential ambiguity: `+0xb0`, `+0xb8`, and `+0xc0` are three separate fields. The view-state reference is not itself proof of the interface pointer's concrete object or the upscaler receiver.

## 3. Source correlation — UE4.27.2 candidate

Public source examined:
- `Engine/Source/Runtime/Engine/Public/SceneView.h`
- `Engine/Source/Runtime/Engine/Public/SceneManagement.h`
- `Engine/Source/Runtime/Renderer/Private/ScenePrivate.h`
- `Engine/Source/Runtime/Renderer/Private/PostProcess/PostProcessUpscale.h`

The public `FSceneViewInitOptions` declares `FSceneViewStateInterface* SceneViewStateInterface`. Its default constructor initializes it to null.

The public `FSceneViewStateInterface` base contains its virtual API and persistent-view-state bookkeeping (eye-adaptation validity, parent pointer, child count). The public `FSceneViewState` implementation derives from that interface and `FRenderResource`. Neither of those public candidate declarations contains an `ISpatialUpscaler*` member at the location under investigation.

This is a key source mismatch: the target binary reads a pointer at `SceneViewStateInterface +0xc0`, while the inspected stock UE4.27.2 declarations do not expose such a field. The target therefore likely contains a BMI-specific renderer modification, a different private implementation, or another concrete view-state implementation. Do not infer that it is the stock `FSceneViewState` layout.

## 4. Virtual call ABI matches ISpatialUpscaler::AddPasses

The UE4.27.2 candidate header `PostProcessUpscale.h` declares:

`FScreenPassTexture ISpatialUpscaler::AddPasses(FRDGBuilder&, const FViewInfo&, const ISpatialUpscaler::FInputs&) const`

The target call site prepares:
- `x0`: receiver pointer loaded from the view-state object's `+0xc0` field;
- `x1`: `FRDGBuilder&`;
- `x2`: `FViewInfo const&`;
- `x3`: address of the `FInputs`-compatible structure;
- `x8`: indirect return storage for the `FScreenPassTexture` aggregate.

That register/argument pattern is consistent with the declared `ISpatialUpscaler::AddPasses` signature and AArch64 aggregate-return ABI. This is substantially stronger than merely guessing from the vtable offset. The precise vtable slot identity should still be confirmed against the target's own vtable/relocations or a matching source build.

## 5. What remains unproven

1. Which function writes the target's `FSceneViewInitOptions +0xb0` field.
2. Which concrete class implements that `FSceneViewStateInterface` in the Android game.
3. Which function writes the pointer at concrete-object `+0xc0`, and its ownership/thread lifetime.
4. Whether the target's vtable layout exactly matches the public candidate's `ISpatialUpscaler` ABI.
5. A safe binary insertion/loader strategy for adding new code to this existing ELF.

## 6. Next audit

Run 12 fixes the incorrect regex and scans all disassembled instructions for `+0xb0`, `+0xb8`, `+0xc0`, `+0xc8`, and `+0xd0` memory references, with separate pointer-store candidate files for `+0xb0` and `+0xc0`. It also includes the `FSceneViewInitOptions` copy/move constructors, `FRendererModule::AllocateViewState`, and `CreateAndInitSingleView`.

The next go/no-go decision depends on those results. A working ABI-level receiver is now strongly identified, but the producer and object lifetime are not yet proven.

No engine bytes were patched and no modified APK was built by runs 10 or 11.