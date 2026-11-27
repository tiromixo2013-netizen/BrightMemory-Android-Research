# Run 14 full-text offset scan findings

Run: 46004172195
Artifact: 15199686836
Original library: libUE4.so, ARM64, original APK SHA256 previously recorded in native-binary-audit-run8.md.

## Scan result

The full .text disassembly scan covered 1,095,837,426 bytes. The corrected scan produced:
- 159,415 lines in view-state-offset-xrefs.txt
- 19,508 candidate loads mentioning displacement +0xB0
- 15,956 candidate stores mentioning displacement +0xB0
- 12,107 candidate stores mentioning displacement +0xC0

These are raw displacement matches, not semantically resolved references. Many are stack spills, vector fields, unrelated structures, or compiler-generated code. They must be narrowed by function boundaries, register provenance, and caller dataflow.

## Confirmed receiver chain from run 11 disassembly

1. ULocalPlayer::CalcSceneViewInitOptions stores a separate result at FSceneViewInitOptions +0xC0 from a virtual call through [LocalPlayer +0x30], vtable byte offset +0x6c8.
2. FSceneView::FSceneView(FSceneViewInitOptions const&) loads options +0xB0 and stores it to the first word of FSceneView.
3. AddMobilePostProcessingPasses loads the first word of FViewInfo, reads +0xC0 from that pointed-to object, null-checks it, and if non-null calls vtable byte offset +0x20.

The options +0xC0 value is copied separately into FSceneView +0x240. It is not the receiver object's +0xC0 spatial-upscaler member.

## Important scan limitation

The full scan intentionally collects broad raw matches. It does not identify which of the thousands of stores target FSceneViewInitOptions. A store such as [x19,#0xb0] is only relevant if x19 is proven to be the options object at that point. Likewise a +0xc0 store cannot be assumed to initialize the spatial-upscaler receiver.

The run 14 workflow's caller-symbol search for CalcSceneViewInitOptions returned empty because the demangled symbol formatting did not match the grep expression. This is another filter false negative, not evidence that there are no callers.

## Next narrowing pass

- Correctly capture the FSceneViewInitOptions copy/move constructors and all direct callers of CalcSceneViewInitOptions and FSceneView construction.
- Trace the actual value passed in options +0xB0 backward through the caller's registers and stack object.
- Identify the concrete type behind that pointer and follow its construction/destruction and reference management.
- Keep the independent LocalPlayer virtual call at vtable +0x6c8 separate until its return type is identified.
- Do not patch the binary until the receiver owner/lifetime and vtable +0x20 ABI contract are proven.

No engine bytes were modified and no patched APK was produced by this scan.
