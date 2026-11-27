# Run 9: FViewInfo and scene-view-state lifecycle disassembly

Date: 2026-11-27  
Source: original Bright Memory: Infinite v1.34 ARM64 APK, verified by the workflow against the original APK SHA-256.  
Workflow: https://github.com/tiromixo2013-netizen/BrightMemory-Android-Research/actions/runs/46001316804  
Artifact: brightmemory-native-hook-audit-full (run 9).

## Confirmed symbols and function ranges

The ELF symbol table confirms these exact entries:

- `FViewInfo::Init()`: RVA `0x79e0948`, size 820.
- `FViewInfo::FViewInfo(FSceneView const*)`: RVA `0x79e0c7c`, size 1596.
- `FViewInfo::FViewInfo(FSceneViewInitOptions const&)`: RVA `0x79e030c`, size 1596.
- `FSceneViewState::FSceneViewState()`: RVA `0x7981ad4`, size 2736.
- `FSceneViewState::Destroy()`: RVA `0x79d6058`, size 936.
- `FSceneViewStateReference::Allocate()`: RVA `0x8b348f8`, size 88.
- `FSceneViewStateReference::Destroy()`: RVA `0x8b34880`, size 84.
- `FSceneViewStateReference::AllocateAll()`: RVA `0x8b349a0`, size 68.
- `FSceneViewStateReference::DestroyAll()`: RVA `0x8b3495c`, size 68.

## What the disassembly proves

1. `AddMobilePostProcessingPasses` still contains the same receiver dispatch at `0x7888fc4`:
   - load the first pointer-sized field from the `FViewInfo` argument into `x10`;
   - load a pointer-sized value from `[x10 + 0xc0]` into `x0`;
   - if non-null, load its vtable and call slot `+0x20` with `FRDGBuilder&`, `FViewInfo const&`, input data and output storage;
   - if null, call `ISpatialUpscaler::AddDefaultUpscalePass`.
2. `FViewInfo::Init` resets numerous internal fields, but does not show a pointer-sized store to `[x10 + 0xc0]` or a direct initialization of the dispatch receiver field.
3. The two `FViewInfo` constructors were disassembled. Their visible scalar stores at offsets rendered as `#0xc0` are 32-bit writes through an interior pointer/register, not evidence of a pointer-sized write to the receiver slot. They do not, by themselves, identify the owner of the receiver pointer.
4. `FSceneViewState::FSceneViewState` initializes the scene-view-state object and its internal data, but the targeted disassembly does not establish that it writes the receiver pointer consumed by `AddMobilePostProcessingPasses`.
5. `FSceneViewStateReference::Allocate` calls a renderer-module virtual method (slot `+0x78`) and stores its returned pointer at `[this + 0x8]`; it also links the reference into an intrusive list. This proves a scene-view-state allocation route exists, but does not prove that this is the exact pointer read by the upscaler dispatch.

## Conclusion

Run 9 improves lifecycle coverage but does **not** yet prove that the receiver at `[*(FViewInfo*) + 0xc0]` is an `FSceneViewState`, nor does it reveal the writer that populates that field. Do not patch the field or vtable based on this evidence.

The next required step is a whole-`.text` reference scan / targeted xref audit around pointer-sized loads and stores at `+0xc0`, followed by tracing the first field of `FViewInfo` back through `FSceneView` construction and scene-view-state assignment. Include relocation/symbol evidence where available. Only after that should we select an in-binary integration point.

No engine bytes have been patched, no new native upscaler has been installed, and no APK has been rebuilt in run 9.