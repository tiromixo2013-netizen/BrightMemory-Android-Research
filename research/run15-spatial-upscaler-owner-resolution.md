# Run 15 follow-up — resolving the spatial upscaler receiver owner

Date: 2026-11-27  
Target: Bright Memory: Infinite Android v1.34, ARM64  
Binary: `libUE4.so`, Build ID `09ae6051d345ede937255d6bd76abefbf6d2b7e8`

## Resolution

The owner of the receiver read in `AddMobilePostProcessingPasses` is the **FSceneViewFamily**, not an unidentified object and not `FSceneViewState`.

The dataflow is:

1. `FSceneViewInitOptions` offset `+0xb0` is the `FSceneViewFamily*` field.
2. `FSceneView::FSceneView(FSceneViewInitOptions const&)` at `0x8b35598` copies it:
   - `0x8b355e8: ldr x8, [x1, #0xb0]`
   - `0x8b355f4: str x8, [x0]`
3. `FRendererModule::CreateAndInitSingleView(FRHICommandListImmediate&, FSceneViewFamily*, FSceneViewInitOptions const*)` at `0x79e85e8` constructs `FViewInfo` from those options and then appends the view to the family.
4. `AddMobilePostProcessingPasses` at `0x78870c0` reads the family pointer from the first word of FViewInfo, then reads the family member at `+0xc0`:
   - `0x7888fb4: ldr x10, [x19]`
   - `0x7888fc4: ldr x0, [x10, #0xc0]`
5. If non-null, it dispatches the receiver's vtable slot at byte offset `+0x20`:
   - `0x7888fe0: ldr x8, [x0]`
   - `0x7888ff0: ldr x9, [x8, #0x20]`
   - `0x7888ff8: blr x9`
6. If null, it falls back to `ISpatialUpscaler::AddDefaultUpscalePass` at `0x78fae30`.

## Family initialization evidence

`FSceneViewFamily::FSceneViewFamily(ConstructionValues const&)` at `0x8b3b840`, size 728 bytes (`0x2d8`), initializes a 32-byte range starting at family offset `+0xb0` to zero:

- `0x8b3b9a0: stp q0, q0, [x19, #0xb0]`

That covers offsets `+0xb0` and `+0xc0`, so the receiver member begins null in a newly constructed family. The field is populated later or remains null; the constructor alone does not identify the setter.

## What is now resolved, and what remains open

Resolved with high confidence:
- The receiver owner is `FSceneViewFamily`.
- The receiver slot is at family byte offset `+0xc0`.
- The receiver is consumed as an `ISpatialUpscaler`-compatible interface because the engine calls its vtable slot `+0x20` and otherwise uses `ISpatialUpscaler::AddDefaultUpscalePass`.
- The correct integration scope is the view-family setup phase, before `AddMobilePostProcessingPasses`, not the LocalPlayer view-state path.

Not yet proven from this binary evidence:
- The exact C++ declared type/layout at family `+0xc0` (raw pointer versus smart-pointer wrapper whose first word is the interface pointer).
- The exact setter or assignment site, and whether ownership is intrusive/shared/ref-counted.
- The precise vtable method signature and all required render-graph invariants.

## Safe engineering conclusion

We have now resolved the receiver's owner and field location. This is a meaningful integration seam, but directly writing an arbitrary pointer to `FSceneViewFamily+0xc0` is not safe until its declared storage type, reference-count operations, and setter/lifetime are verified. The safe next step is to trace family setup/extension callbacks and any smart-pointer construction/copy/destruction around this field, then implement through the verified ownership path. Do not patch the field with a naked pointer.

No binary bytes have been changed and no modified APK has been produced by this follow-up.
