# Run 17 — spatial upscaler family lifetime evidence

Workflow: https://github.com/tiromixo2013-netizen/BrightMemory-Android-Research/actions/runs/46006624300
Commit: `f1627e7196cb42bb8285563fb499cc655909b700`
Artifact ID: `15200438970`
Artifact: `brightmemory-native-hook-audit-full`
Result: workflow succeeded; original APK reassembly/hash verification and complete-function disassembly succeeded.

## Findings

### Copy constructor — `FSceneViewFamily::FSceneViewFamily(FSceneViewFamily const&)`
RVA `0x79e5a04`, size 412 bytes.

The constructor loads a 16-byte vector from source offset `+0xb8` and stores it at destination offset `+0xb8`. This copies the pointer at `+0xc0` (the primary spatial upscaler interface) along with its neighboring field. It also separately copies `+0xc8`. This is not a null-reset or omitted field.

### Destructor — `FSceneViewFamily::~FSceneViewFamily()`
RVA `0x8b3bb18`, size 276 bytes.

The destructor checks offsets `+0xb0`, `+0xc0`, and `+0xc8`. Each non-null interface receives a virtual call through vtable byte offset `+0x8`. This establishes that the family destructor performs virtual cleanup on the primary spatial upscaler pointer. Do not independently delete/free the pointer or run a duplicate cleanup path.

### Renderer constructor — `FSceneRenderer::FSceneRenderer(FSceneViewFamily const*, FHitProxyConsumer*)`
RVA `0x79e5618`, size 1004 bytes.

The renderer constructs its embedded `FSceneViewFamily` at `this+0x10` by calling the copy constructor above. Therefore the renderer-owned family receives the copied primary spatial upscaler pointer and later follows the same family destructor cleanup path.

## What this resolves

The main structural lifetime uncertainty is resolved: the primary interface pointer is copied into the renderer-owned family and the family destructor dispatches virtual cleanup for it. This is consistent with the intended primary spatial upscaler lifecycle and supports the upstream Qualcomm integration model.

It does not prove exact target ABI compatibility, the exact semantic name of vtable slot `+0x8`, exact plugin binary compatibility, or shader compatibility. The safe integration remains an adapter built for the target engine ABI and its native renderer path.

## Evidence artifact

The full disassembly and symbol evidence are retained in artifact `15200438970`. In particular, `complete-hook-disassembly.txt` contains all three complete function ranges.
