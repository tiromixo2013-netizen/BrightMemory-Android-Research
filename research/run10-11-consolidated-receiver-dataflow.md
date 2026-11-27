# Run 10–11 consolidated receiver dataflow findings

Date: 2026-11-27
Branch: native-integration
Evidence: artifacts from workflow runs 46002239668 (run 10) and 46002454836 (run 11), disassembly of the original ARM64 libUE4.so.

## Executive result

The key distinction is now established: the +0xC0 read used by mobile post-processing is NOT the FSceneViewInitOptions +0xC0 field written by ULocalPlayer::CalcSceneViewInitOptions.

There are two separate objects and offsets in the dataflow:

1. ULocalPlayer::CalcSceneViewInitOptions writes an opaque result to FSceneViewInitOptions +0xC0.
2. FSceneView::FSceneView(FSceneViewInitOptions const&) copies FSceneViewInitOptions +0xB0 into the first machine word of the FSceneView base object.
3. AddMobilePostProcessingPasses receives FViewInfo const&, loads its first machine word, treats that value as a pointer, then reads +0xC0 from the pointed-to object and dispatches its virtual slot +0x20.

Consequently, the +0xC0 field involved in the spatial-upscaler dispatch belongs to the object pointed to by the first word of FViewInfo, not directly to FSceneViewInitOptions and not to FSceneViewStateReference.

## Direct disassembly evidence

### A. ULocalPlayer writes the options fields

Function: ULocalPlayer::CalcSceneViewInitOptions
Address: 0x884a708, size 780 bytes.

At 0x884a944:
- ldr x9, [x8, #0xa8]
- add x9, x9, x20
- ldr x9, [x9, #0x8]
- str x9, [x19, #0xb8]

This stores the selected FSceneViewStateReference-related pointer at options offset +0xB8.

At 0x884a948–0x884a960:
- ldr x0, [x8, #0x30]
- ldr x9, [x0]
- ldr x9, [x9, #0x6c8]
- blr x9
- str x0, [x19, #0xc0]

Thus options offset +0xC0 receives the return value of a virtual call through the object at [LocalPlayer + 0x30], vtable slot offset +0x6c8. This is not evidence that the returned value is the spatial upscaler; it is a separate options field. Its exact semantic type still requires symbol/source confirmation.

### B. FSceneView copies options +0xB0 into its first word

Function: FSceneView::FSceneView(FSceneViewInitOptions const&)
Address: 0x8b35598, size 3424 bytes.

At 0x8b355e8–0x8b355f4:
- ldr x8, [x1, #0xb0]
- str x8, [x0]

Here x1 is the FSceneViewInitOptions const& argument and x0 is the new FSceneView object. This establishes the first-word receiver/container pointer's provenance from options offset +0xB0.

The same constructor separately copies options +0xB8 to FSceneView +0x8, and options +0xC0 to FSceneView +0x240 (instructions around 0x8b356b0–0x8b356bc). These are separate fields and must not be conflated with the receiver pointer's own +0xC0 member.

### C. Mobile post-processing reads receiver +0xC0 and dispatches vtable +0x20

Function: AddMobilePostProcessingPasses
Address: 0x78870c0, size 9616 bytes.

At 0x7888fb4–0x7888ff8:
- ldr x10, [x19] — first machine word of FViewInfo.
- ldr x0, [x10, #0xc0] — receiver object's +0xC0 member.
- Null check at 0x7888fdc.
- If non-null: ldr x8, [x0]; ldr x9, [x8, #0x20]; blr x9.

The call arguments are receiver in x0, render graph builder in x1, view info in x2, input data pointer in x3, and local output storage address in x8. If the receiver member is null, execution branches to the existing default-upscale path.

## Important correction to earlier interpretation

Earlier shorthand that described the dispatch as if it were simply FViewInfo +0xC0 or the same FSceneViewInitOptions +0xC0 was imprecise. The confirmed chain is:

FSceneViewInitOptions +0xB0 → FSceneView first word → FViewInfo first word → dereference receiver +0xC0 → virtual slot +0x20.

The FSceneViewInitOptions +0xC0 field is an independent value returned by a LocalPlayer virtual method at vtable byte offset 0x6c8.

## Why the automated xref reports were empty

The run 10/11 artifacts contain zero-byte offset grep reports. The workflow's grep expressions did not correctly match the actual llvm-objdump output (and the regex was over-escaped). Therefore those empty reports are a tooling false negative, not proof that there are no references or stores. The direct function disassembly above provides the confirmed references and writes listed here.

## What is now known / still unknown

Known:
- The upscaler receiver pointer is sourced through options offset +0xB0.
- The view constructor stores it as the first word of FSceneView.
- The mobile post-processing dispatch reads receiver +0xC0.
- The interface call uses vtable slot byte offset +0x20.
- Null receiver member selects the engine's default spatial-upscale path.

Still unknown:
- The concrete type and owner of the object passed through options +0xB0.
- The code that populates options +0xB0 (writer/call chain).
- The exact ownership, lifetime, thread-affinity and reference-count semantics of that receiver.
- The exact virtual method signature and ABI-level return/parameter contract beyond observed register use.
- Whether the options +0xB0 receiver can safely be substituted or extended in this binary without changing unrelated view behavior.

## Next focused work before binary patching

1. Find all callers/writers of the FSceneViewInitOptions construction path and trace the value placed at +0xB0 back to its concrete source.
2. Resolve the virtual call at [LocalPlayer + 0x30] vtable +0x6c8 independently; do not mislabel its return value as the upscaler.
3. Inspect the concrete receiver's construction, destruction, refcount/lifetime, and thread handoff.
4. Resolve the vtable +0x20 signature using matching engine source/build metadata and binary call-site evidence.
5. Only then design a minimal in-binary hook or receiver substitution with a verified default fallback.

No bytes in libUE4.so were modified and no patched APK was produced by runs 10–11.
