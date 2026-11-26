# Follow-up Trace: FSceneView State Pointer and Upscaler Dispatch

Target: Bright Memory: Infinite Android v1.34, ARM64, `libUE4.so`
Analysis is static disassembly only; runtime ownership is not claimed unless explicitly stated.

## Confirmed copy into FSceneView

At `FSceneView::FSceneView(FSceneViewInitOptions const&)` (`0x8b35598`):

```asm
8b355e8  ldr x8, [x1, #0xb0]
8b355f4  str x8, [x0]
```

So the first pointer in the constructed `FSceneView` is copied from `FSceneViewInitOptions + 0xb0`. The mobile post-processing function later reads that first pointer as its `x10` object.

## Exact dispatch sequence

At `AddMobilePostProcessingPasses` (`0x78870c0`):

```asm
7888fb4  ldr x10, [x19]          // first pointer in FViewInfo/FSceneView base
7888fc4  ldr x0, [x10, #0xc0]   // candidate upscaler object
7888fc8  ldr x9, [x10]
7888fcc  ldr x9, [x9]
7888fd0  cmp x9, x19
7888fdc  cbz x0, 0x7889000
7888fe0  ldr x8, [x0]           // returned object's vtable
7888ff0  ldr x9, [x8, #0x20]   // virtual slot +0x20
7888ff8  blr x9
```

The object loaded from `[x10 + 0xc0]` is passed as the virtual-call receiver. The call receives the RDG builder, view, input structure, and hidden return storage. Null uses the engine's default spatial upscale pass.

## What CalcSceneViewInitOptions does — and does not prove

At `ULocalPlayer::CalcSceneViewInitOptions` (`0x884a708`), the view-state reference array is grown and an entry is allocated:

- `0x884a8dc`: grows the `FSceneViewStateReference` array.
- `0x884a928`: calls `FSceneViewStateReference::Allocate` (`0x8b348f8`).
- `FSceneViewStateReference::Allocate` calls the renderer module virtual slot `+0x78` and stores its returned pointer at reference-object offset `+0x8`.
- `0x884a944`: stores the reference entry's pointer into init-options offset `+0xb8`.
- `0x884a94c–0x884a960`: calls a PlayerController virtual slot `+0x6c8` and stores its return at init-options offset `+0xc0`.

Important: this function does not visibly write init-options offset `+0xb0` in the inspected range. The writes to `+0xb8` and `+0xc0` are separate fields and must not be mistaken for the source of the `+0xb0` pointer copied into `FSceneView`.

## Current conclusion

- The exact consumer/dispatch site is confirmed.
- The source field copied into the view is confirmed: init-options `+0xb0`.
- The exact producer/writer of init-options `+0xb0` is still not identified.
- The exact concrete type and setter for the object member at `+0xc0` are still not identified.
- Do not patch either offset yet. The next trace must follow assignments to init-options `+0xb0`, identify the concrete `FSceneViewStateInterface` implementation, and then trace writes to that object's `+0xc0` member.
