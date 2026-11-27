# Unified rendering path and AMD CAS shader integration map

Date: 2026-11-27
Target: Bright Memory: Infinite Android v1.34, ARM64, existing packaged UE4 binary.

## Run 17 lifetime audit — completed evidence

Workflow run: https://github.com/tiromixo2013-netizen/BrightMemory-Android-Research/actions/runs/46006624300
Artifact: `brightmemory-native-hook-audit-full` (artifact ID 15200438970)
Original binary was reassembled and verified before disassembly. No binary bytes were patched.

### FSceneViewFamily copy constructor

At RVA `0x79e5a04`, the copy constructor copies the 16-byte block beginning at `+0xb8` from source to destination. This includes the primary spatial upscaler pointer at `+0xc0`. It separately copies `+0xc8`. Therefore the renderer's family copy retains the primary upscaler pointer rather than dropping it.

### FSceneViewFamily destructor

At RVA `0x8b3bb18`, the destructor loads pointers at `+0xb0`, `+0xc0`, and `+0xc8`; for each non-null pointer it dispatches the object's vtable entry at byte offset `+0x8`. This is the target's virtual cleanup/release path for those interface slots. Do not manually free these interface pointers or add a second cleanup path.

### FSceneRenderer constructor

At RVA `0x79e5618`, FSceneRenderer invokes `FSceneViewFamily::FSceneViewFamily(FSceneViewFamily const&)` to construct its embedded family copy at `this+0x10`. This confirms the primary spatial upscaler pointer is carried into the renderer-owned family copy and later receives the family destructor's virtual cleanup dispatch.

### Consequence for SGSR1

This resolves the main structural lifetime uncertainty: the primary interface pointer is copied into the renderer's family copy and both family lifetimes use virtual cleanup. It is a strong match for the upstream Qualcomm architecture. It still does not prove exact C++ ABI compatibility, exact vtable slot semantics, plugin binary compatibility, or shader compatibility. The safe route remains an adapter built against matching engine ABI—not dropping a prebuilt UE plugin into the APK.

## AMD FidelityFX CAS source and shader references

Official upstream repository: https://github.com/GPUOpen-Effects/FidelityFX-CAS
Official AMD CAS documentation: https://gpuopen.com/manuals/fidelityfx_sdk/techniques/contrast-adaptive-sharpening/
Original CAS v1 source file: `ffx-cas/ffx_cas.h`
Official sample compute shaders:
- HLSL: `sample/src/DX12/CAS_Shader.hlsl`
- GLSL/Vulkan: `sample/src/VK/CAS_Shader.glsl`

The AMD source describes CAS as a spatial RGB filter with optional scaling. The core is a compute shader; the official sample dispatch uses 64 threads per workgroup and a remapped 8x8 tile pattern. The packed FP16 path processes paired 8x8 tiles; the FP32 path uses the 8x8 tile filter with four tile evaluations. The source's CPU setup routine `CasSetup` generates `const0` and `const1` from sharpness and input/output dimensions.

### Correct CAS role in this project

CAS is a **post-upscale sharpening stage**, not a replacement for SGSR/FSR/MWSR and not a frame-generation technique.

Unified image pipeline:

1. Resolution Controller selects display/native target and render fraction.
2. Graphics Quality selects actual game scalability/effect settings.
3. Render Scale determines the internal scene render size.
4. Exactly one spatial upscaler is selected and run: SGSR1 / SGSR2 / MWSR / FSR Lite / Nearest / Bilinear (only implementations actually available and validated may be exposed).
5. Optional AMD CAS pass sharpens the selected upscaler's output. CAS Sharpness is a separate scalar; CAS Off bypasses the pass.
6. Final color/tone-map and UI composition follow the target engine's actual render ordering. CAS should normally run after tone mapping for SDR linearized input and before UI if the UI must remain untouched; validate the game's color pipeline before choosing the insertion point.
7. Frame pacing and low-latency queue policy control submission/presentation, not image filtering.
8. Frame generation, if implemented, consumes real motion/depth/history and is a separate temporal stage; it must not be faked by duplicating frames.

Only one upscaler is active at a time. CAS may follow any selected upscaler because it is a separate sharpening stage. Do not stack a second upscaler inside CAS in the first integration; use sharpen-only mode to avoid changing output dimensions twice.

### CAS shader integration requirements

- Import AMD's `ffx_cas.h` and its matching portability header `ffx_a.h` from the same upstream source revision. Preserve AMD's MIT copyright and license notice in redistributed source.
- Compile a dedicated compute shader permutation for the target RHI and precision. Do not assume the sample DX12 shader compiles unchanged for Android.
- For Vulkan, adapt the official GLSL compute sample's descriptor bindings to the engine's RDG resources and target Vulkan shader compiler.
- For OpenGL ES, the official CAS sample does not provide an ES-specific shader path. Do not mark GLES as supported until a compatible GLSL ES implementation is compiled and device-tested.
- Supply a linear RGB input in the expected normalized range. If the game buffer is gamma/sRGB, perform a correct linearization path or use the supported conversion mechanism; blindly sharpening gamma-space color can over-sharpen and create halos.
- Generate `const0/const1` from actual input/output dimensions and sharpness via `CasSetup`. For sharpen-only, input and output dimensions match.
- Use bounds-safe dispatch dimensions and valid source/destination resources. Do not read and write the same texture in-place unless the selected shader/API explicitly supports it; use separate RDG input/output textures.
- Prefer FP16 only after confirming Vulkan format/storage support and device shader support; retain an FP32 fallback. The Adreno/Mali hardware must be tested independently.
- CAS sharpness control range is 0..1 per AMD sample documentation. Map the UI slider to the shader setup parameter; do not implement it as a cosmetic setting.
- Add an explicit CAS enable switch and restore the unmodified engine path when disabled.

## Integration boundary for the existing BMI APK

The research repository currently contains the original APK and binary-analysis workflows, not the BMI engine source tree or a matching UE4 Android build toolchain. A CAS shader source file in the repository is not enough to make the existing `libUE4.so` execute it. The shader must be compiled for the game's exact RHI/UE shader platform, registered/loaded by a native renderer integration, bound to valid RDG resources, and packaged in the format the existing engine accepts. A cooked shader library or native module cannot safely be fabricated from the source-only sample.

No APK modification is claimed by this report. The next concrete gate is to identify the game's active Android RHI and exact UE shader platform/format, then build the CAS shader permutation and renderer pass against the matching engine ABI. Vulkan and GLES must be treated as separate validation targets.

## Status

- [x] SGSR1 upstream architecture inspected.
- [x] BMI spatial-upscaler dispatch seam identified.
- [x] Run 17 family-copy, destructor, and renderer-constructor audit completed.
- [x] AMD CAS upstream core and HLSL/GLSL compute samples identified.
- [x] Unified stage ordering defined; CAS kept separate from upscaler selection.
- [ ] Exact BMI engine ABI/changelist identified.
- [ ] Android RHI and shader platform confirmed.
- [ ] CAS shader compiled for the actual target platform.
- [ ] Native renderer pass connected to RDG resources.
- [ ] APK repacked, installed, and runtime-tested.

## Source staging completed

The official CAS source is now vendored under `vendor/amd-fidelityfx-cas/` from upstream commit `9fabcc9a2c45f958aff55ddfda337e74ef894b7f`:
- `ffx_a.h`
- `ffx_cas.h`
- `CAS_Shader.hlsl`
- `CAS_Shader.glsl`
- `LICENSE.txt`

A GitHub Actions validation workflow was added at `.github/workflows/cas-shader-validation.yml`. Run `46007752864` confirmed the official GLSL sample compiles, while HLSL failed because the sample expects `WIDTH/HEIGHT/DEPTH` compile-time defines. The workflow was corrected to pass a 64-thread workgroup (`64,1,1`) and rerun as `46007813620`; this rerun is still in progress at the time of this report update. A successful reference compile is not the same as a UE4 Android shader build.

## Reference shader compile validation — passed

Workflow run: https://github.com/tiromixo2013-netizen/BrightMemory-Android-Research/actions/runs/46007831390
Artifact ID: `15200694503`
Artifact: `amd-cas-reference-shader-validation`

Both the official GLSL compute sample and HLSL compute sample compiled to SPIR-V, and both binaries passed `spirv-val --target-env vulkan1.0`.

SHA-256:
- HLSL FP32 sharpen-only SPIR-V: `2ecd2012302f4cdf1efc4c0a4abecc54b43fe66e958a7a5b0580474600542e29`
- GLSL FP32 SPIR-V: `afdc874571c2f012da05931ef8b0545986d663d11ea8d98fea5f98fb8c2310f0`

This validates the upstream reference shader source and the portable shader compiler path only. It is not a UE4 Android shader library, does not validate the BMI render graph bindings, and is not proof of execution on the phone GPU.
