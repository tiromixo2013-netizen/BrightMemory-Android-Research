# Native binary audit — run 8 findings

Target: Bright Memory: Infinite Android v1.34, ARM64 `libUE4.so`
Build ID: `09ae6051d345ede937255d6bd76abefbf6d2b7e8`
Audit artifact: `brightmemory-native-hook-audit-full` (GitHub Actions run 8)
Method: symbol-bounded AArch64 disassembly; evidence gathering only. No binary bytes modified.

## Verified facts

1. `AddMobilePostProcessingPasses` is present at RVA/VMA `0x78870c0`, symbol size 9,616 bytes.
2. Inside it, the code has a conditional custom-upscaler call path at `0x7888fdc–0x7888ffc`: it loads an interface/object candidate, checks for null, obtains a virtual function at vtable offset `0x20`, and calls it indirectly. If null, it calls `ISpatialUpscaler::AddDefaultUpscalePass` at `0x78fae30`. This proves a custom dispatch branch exists, but does **not** yet prove the source/owner/initialization of the object loaded on this path.
3. `ISpatialUpscaler::AddDefaultUpscalePass` exists at `0x78fae30`, size 1,424 bytes.
4. `FSceneViewExtensions::ForEachActiveViewExtension` exists at `0x8b3c1bc`, size 400 bytes. This proves the engine has the extension iteration routine, not that a plugin can safely be injected without establishing registration and lifecycle.
5. Resolution functions are present: `FSceneRenderer::ApplyResolutionFraction` at `0x79e5ba0` (36 bytes), `FSceneRenderer::GetDesiredInternalBufferSize` at `0x79e5c10` (144 bytes), and `UGameUserSettings::SetResolutionScaleValue` at `0x8735a9c` (164 bytes), with float variant at `0x8735b40` (168 bytes).
6. The ELF is AArch64 Android, NDK r27c, Build ID above. The binary is stripped, though a large dynamic symbol table is present.

## Next reverse-engineering task

Trace the dataflow that supplies the custom-dispatch receiver used at `0x7888fc4`, including its owning structure, writer/initializer, lifetime, and any registration function. Use cross-references and relocation/dynamic-symbol evidence; do not infer a struct offset from the single call site. Then validate the spatial upscaler interface ABI against this exact engine build before writing any hook.

Separately trace how the game's existing graphics menu invokes the resolution-scale setters and persists settings. This can establish a safe first feature without patching engine internals.

## Not yet established

- No custom upscaler has been registered or executed in the game.
- No proof yet that SGSR1/SGSR2/FSR integration is ABI-compatible with this binary.
- No frame-generation implementation or motion-vector path has been verified.
- No Vulkan queue-depth or latency change has been made.
- No cooked UMG menu has been changed.
- No modified APK has been built.

## Safety rule

Do not patch the indirect-call site, vtable, or guessed object offset directly. Preserve the default-upscaler fallback and establish object construction, ownership, thread affinity, and teardown first.
