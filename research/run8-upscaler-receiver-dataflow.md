# Run 8 follow-up: receiver provenance

This note narrows the unresolved custom-upscaler receiver using the actual run-8 disassembly.

## Exact call-site dataflow

At `AddMobilePostProcessingPasses`:

```asm
7888fb4: ldr x10, [x19]
7888fc4: ldr x0, [x10, #0xc0]
7888fdc: cbz x0, 0x7889000
7888fe0: ldr x8, [x0]
7888ff0: ldr x9, [x8, #0x20]
7888ff8: blr x9
7889000: ... fallback setup ...
7889020: bl ISpatialUpscaler::AddDefaultUpscalePass
```

Here `x19` is the `FViewInfo const&` argument, and `x10` is the first machine word loaded from that object. The custom receiver is therefore loaded from **the object addressed by the first word of FViewInfo, at offset +0xC0**. This is a more precise description than saying it is a field at `FViewInfo+0xC0`, or that it is definitely a `FSceneViewState` field. The exact pointed-to type and ownership remain unknown.

The receiver's vtable slot at byte offset `0x20` is invoked with:
- x0: receiver
- x1: `x24` (FRDGBuilder&)
- x2: `x19` (FViewInfo const&)
- x3: address of local `ISpatialUpscaler::FInputs`
- x8: output/local result storage address

The null branch calls the engine's default spatial-upscale pass. Preserve this fallback in any implementation.

## Required next evidence

1. Identify the concrete type of the first-word object used by `FViewInfo` (likely a vtable-backed view-related object, but do not assume).
2. Find all writes to byte offset `+0xC0` on that concrete object and trace constructor/copy/destructor paths.
3. Identify the virtual method occupying vtable byte offset `0x20` and compare its full signature/ABI with the interface expected by this call site.
4. Check the engine's registered scene-view-extension lifecycle independently; do not conflate it with the receiver's ownership.
5. Only after those facts are established, decide between a native extension, a narrow binary hook, or a cooked-menu setting bridge.

## Guardrails

No guessed struct offsets, no direct vtable patch, no replacement of the fallback, and no claim of runtime upscaler support based only on this call site. No APK changes were made in this follow-up.
