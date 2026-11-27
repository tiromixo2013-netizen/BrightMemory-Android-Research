# Run 15 — native hook audit results

Date: 2026-11-27  
Repository: `tiromixo2013-netizen/BrightMemory-Android-Research`  
Branch: `native-integration`  
Workflow run: [46004795349](https://github.com/tiromixo2013-netizen/BrightMemory-Android-Research/actions/runs/46004795349)  
Artifact: `brightmemory-native-hook-audit-full` (ID `15202200455`)

## Run status

The workflow completed successfully. Reassembly and verification of the original APK succeeded; the full disassembly and evidence artifact uploaded successfully.

The inspected `libUE4.so` is AArch64 Android, Build ID `09ae6051d345ede937255d6bd76abefbf6d2b7e8`, SHA-256:

`94ee451ed42c4aee4c885aac3e6b6ccef2d3ab109e01a86b10e527c32c82b35e`

The full .text disassembly was 1,095,837,426 bytes. Candidate reports contain 159,415 combined b0/c0/b8-style offset references in the broad scans (the dedicated offset xref file has 115,499 lines), 15,956 b0 stores, 12,107 c0 stores, and 19,508 b0 loads.

## Confirmed receiver dispatch

In `AddMobilePostProcessingPasses` at RVA `0x78870c0`:

- `0x7888fb4: ldr x10, [x19]`
- `0x7888fc4: ldr x0, [x10, #0xc0]`
- If non-null, the receiver vtable is loaded and slot byte offset `+0x20` is called at `0x7888ff8`.
- Arguments at the indirect call: x0 receiver, x1 FRDGBuilder reference, x2 FViewInfo reference, x3 stack-resident inputs; x8 points to output/local storage.
- If null, the function calls `ISpatialUpscaler::AddDefaultUpscalePass` at `0x78fae30`.

This is not a direct `FViewInfo + 0xc0` access. The first pointer-sized field of FViewInfo is dereferenced first, then +0xc0 is read from that pointed-to object.

## Confirmed view construction chain

In `FSceneView::FSceneView(FSceneViewInitOptions const&)` at `0x8b35598`:

- `ldr x8, [x1, #0xb0]`
- `str x8, [x0]`

So `FSceneViewInitOptions +0xb0` is copied into the first machine word of FSceneView, inherited at the start of FViewInfo. This establishes the structural path to the receiver lookup above.

Separately, `ULocalPlayer::CalcSceneViewInitOptions` at `0x884a708` writes options +0xb8 and +0xc0 through a separate path. In particular, options +0xc0 is populated from a virtual call through LocalPlayer+0x30, vtable byte offset +0x6c8. The FSceneView constructor copies options +0xc0 to FSceneView +0x240. It must not be conflated with the +0xb0 receiver chain.

## What this run did not prove

- It did not identify the exact concrete type, owner, or lifetime/ref-counting rules of the object reached through options +0xb0.
- It did not resolve the exact C++ signature/ABI and semantics of receiver vtable slot +0x20.
- Broad +0xb0/+0xc0 candidate stores remain noisy; many are unrelated objects or stack spills.
- The dedicated pointer-store and CalcSceneView caller grep files are empty because their matching expressions did not match the disassembler's actual output format. Empty files are not evidence that no such stores/callers exist.
- No bytes in `libUE4.so` were patched. No upscaler, CAS, frame generation, frame pacing, low-latency, overlay, or UI feature was integrated or runtime-tested. No modified APK was produced.

## Engineering decision

The receiver dispatch is a credible integration seam for a single spatial upscaler, but it is not yet a safe patch point. Before modifying it, resolve the +0xb0 object's concrete type and lifecycle, identify the exact interface ABI at vtable +0x20, and verify the target engine ABI. Keep the existing null/default-upscaler fallback intact.

Frame generation, Vulkan queue changes, and the UI/settings integration are separate workstreams; they cannot honestly be marked implemented based on this binary audit.
