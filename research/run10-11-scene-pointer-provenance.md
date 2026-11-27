# Run 10–11: scene-view pointer provenance and offset scan review

Date: 2026-11-27  
Target: Bright Memory: Infinite Android v1.34, ARM64  
Original APK SHA-256: `c35b05ac7b19fd087844de2b6261d33fbe103a6a5714c5e42d6101fab07881e4`  
Library: `lib/arm64-v8a/libUE4.so`, ELF Build ID `09ae6051d345ede937255d6bd76abefbf6d2b7e8`

## Run status and evidence

- Run 10: `46002239668`, succeeded. Artifact `15198144465`.
- Run 11: `46002454836`, succeeded. Artifact `15199395842`.
- Both runs reassembled the original APK and passed its SHA-256 verification.
- Both artifacts were downloaded and inspected. Run 10's `offset-c0-xrefs.txt` and pointer-store candidate report are empty; this is **not** proof that no stores exist. The scan did not produce usable cross-reference evidence and must be replaced with a validated whole-text scan.

## Confirmed constructor dataflow

From run 11 disassembly of `FSceneView::FSceneView(FSceneViewInitOptions const&)` at RVA `0x8b35598`:

```asm
8b355e8: ldr x8, [x1, #0xb0]
8b355f4: str x8, [x0]
```

This proves the first pointer-sized field of the constructed `FSceneView` is copied directly from `FSceneViewInitOptions + 0xb0`. The upscaler consumer reads the receiver from `[FSceneView + 0xc0]` only after dereferencing that first field. Therefore the concrete dataflow is:

`FSceneViewInitOptions + 0xb0 -> FSceneView[0] -> *(FSceneView[0] + 0xc0) -> virtual slot +0x20`.

This is more precise than describing the receiver as a direct `FViewInfo + 0xc0` field.

## What ULocalPlayer::CalcSceneViewInitOptions actually writes

The complete disassembly of `ULocalPlayer::CalcSceneViewInitOptions` at RVA `0x884a708` shows:

- It allocates/maintains an `FSceneViewStateReference` array.
- It calls `FSceneViewStateReference::Allocate()` when the indexed reference must be created.
- It stores the reference's object pointer at `FSceneViewInitOptions + 0xb8` (`0x884a944`).
- It calls a PlayerController virtual method and stores its return value at `FSceneViewInitOptions + 0xc0` (`0x884a960`).
- It writes other unrelated view-init fields such as `+0xc8`, `+0xd0`, etc.
- In the inspected complete function, it does **not** write `FSceneViewInitOptions + 0xb0`.

Consequently, do not confuse the `+0xb8` scene-view-state reference or `+0xc0` PlayerController result with the separate `+0xb0` value copied into `FSceneView[0]`.

## Copy constructor

`FSceneView::FSceneView(FSceneView const&)` at RVA `0x7981330` begins by copying the first 16 bytes from the source view to the destination. This preserves the first pointer-sized field but does not identify its original producer.

## Updated conclusion

We have proven the *transport* of the candidate receiver's owner pointer from view-init options into `FSceneView`, and excluded `ULocalPlayer::CalcSceneViewInitOptions` as the writer of the `+0xb0` field in the inspected function.

We have **not** proven:
- which call site or view extension writes `FSceneViewInitOptions + 0xb0`;
- the concrete type of the object addressed by that pointer;
- the object's lifetime/thread ownership;
- the exact C++ signature and ABI of the virtual method at receiver vtable slot `+0x20`.

No patch to the ELF, no upscaler injection and no modified APK have been produced.

## Next required audit

1. Validate a full `.text` disassembly scan using the actual installed objdump output format, and retain all candidate stores to offsets `+0xb0` (and pointer-producing stores/copies into the options object).
2. Trace the callers of `FSceneViewInitOptions` creation and all view-extension `SetupView` / `BeginRenderViewFamily` paths that receive mutable view/options state.
3. Inspect `FSceneViewInitOptions` copy/move construction and all relevant writes before `FSceneView` construction.
4. Only then determine whether a supported view-extension registration route can set the primary spatial upscaler without modifying the receiver object's layout or vtable.

This is the current evidence boundary; do not jump directly to a binary patch.