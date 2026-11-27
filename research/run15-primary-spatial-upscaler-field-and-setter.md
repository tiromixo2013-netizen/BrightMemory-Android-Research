# Run 15 follow-up — spatial upscaler field type and setter

Date: 2026-11-27  
Target binary: Bright Memory: Infinite Android v1.34, ARM64 `libUE4.so` Build ID `09ae6051d345ede937255d6bd76abefbf6d2b7e8`

## Determination

The field read at `FSceneViewFamily + 0xc0` matches Unreal's `PrimarySpatialUpscalerInterface`, whose declared type in the public `FSceneViewFamily` API is a raw `ISpatialUpscaler*`, not `TRefCountPtr<ISpatialUpscaler>`.

The public API exposes:

```cpp
void SetPrimarySpatialUpscalerInterface(ISpatialUpscaler* InSpatialUpscalerInterface);
const ISpatialUpscaler* GetPrimarySpatialUpscalerInterface() const;
```

The setter contract is: non-null input, and no primary spatial upscaler already assigned to the family. The implementation assigns the raw pointer to `PrimarySpatialUpscalerInterface`. This matches the binary evidence:
- `FSceneViewFamily` constructor at `0x8b3b840` zero-initializes a 32-byte region at `+0xb0`, including `+0xc0`.
- `AddMobilePostProcessingPasses` at `0x78870c0` loads the pointer from family `+0xc0`, null-checks it, and dispatches virtual slot `+0x20`.
- The call's AArch64 argument registers are consistent with the interface's `AddPasses(FRDGBuilder&, const FViewInfo&, const FInputs&) const` method: x0=this, x1=GraphBuilder, x2=View, x3=Inputs. x8 is used as the indirect result location for the returned screen-pass texture.

## Correct integration point

The established Unreal extension lifecycle is:
1. Register an `ISceneViewExtension`.
2. In `BeginRenderViewFamily(FSceneViewFamily&)`, verify the view family is eligible and no primary/temporal upscaler conflict exists.
3. Call `InViewFamily.SetPrimarySpatialUpscalerInterface(new ConcreteSpatialUpscaler(...))`.
4. The renderer later reads that interface during mobile post-processing and calls its `AddPasses` implementation.

This is the supported integration seam; it is not necessary or appropriate to write directly to `family + 0xc0`.

## Lifetime: remaining binary-specific proof

The field is a raw pointer, so there is no intrusive reference count stored in the field itself. The interface object must remain alive for the view-family/render lifecycle and must satisfy the engine's fork/lifetime contract. Unreal's renderer commonly forks interface instances for renderer-side use through `Fork_GameThread(ViewFamily)`.

However, this audit has not yet disassembled the target binary's `FSceneViewFamily` copy constructor (`0x79e5a04`) and destructor (`0x8b3bb18`) or the `FSceneRenderer` constructor (`0x79e5618`) to establish precisely who deletes the original and forked raw pointers in this particular build. Therefore, do not assume that the family destructor owns/deletes the interface, and do not install a pointer with an unverified lifetime strategy.

## Version and ABI qualification

The raw-pointer member and setter contract are corroborated by the public Unreal `FSceneViewFamily` API and MobileFSR's documented `BeginRenderViewFamily` integration pattern. The target binary's exact engine changelist/source revision has not yet been matched to that source snapshot. The field offset and dispatch path are binary-confirmed; the C++ API contract is source corroboration, not proof of exact source revision identity.

## Engineering conclusion

Resolved:
- field identity: `PrimarySpatialUpscalerInterface`
- declared API type: raw `ISpatialUpscaler*`
- supported setter: `SetPrimarySpatialUpscalerInterface`
- lifecycle hook: `ISceneViewExtension::BeginRenderViewFamily`
- dispatch: `AddPasses` via vtable slot byte offset `+0x20`

Still open before a safe patch:
- target-build copy/destructor ownership behavior and exact fork cleanup
- exact target-build vtable slot/signature confirmation against the matched engine source/changelist
- implementation and runtime verification of a concrete upscaler

No binary bytes have been modified; no modified APK has been produced.
