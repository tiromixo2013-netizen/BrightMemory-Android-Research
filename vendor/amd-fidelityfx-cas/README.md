# AMD FidelityFX CAS — vendored upstream source

These files are copied from AMD's official repository:
https://github.com/GPUOpen-Effects/FidelityFX-CAS

Pinned upstream commit: `9fabcc9a2c45f958aff55ddfda337e74ef894b7f`

Files:
- `ffx_a.h` — AMD's shader portability header.
- `ffx_cas.h` — CAS algorithm and CPU/GPU setup/filter functions.
- `CAS_Shader.hlsl` — official HLSL compute sample.
- `CAS_Shader.glsl` — official GLSL/Vulkan compute sample.
- `LICENSE.txt` — AMD MIT license.

The two headers are a matched pair and must remain from the same upstream revision. The sample shaders are reference implementations, not yet Bright Memory UE4 shaders. Their descriptor bindings, resource access, dispatch dimensions, shader compiler profile, and bounds handling must be adapted to the game's exact RHI and UE4 shader pipeline before use.

## Bright Memory integration contract

- Run CAS as an optional sharpen-only compute pass after the selected spatial upscaler.
- Use separate input/output RDG textures; do not alias read and write resources without explicit API support.
- Compute `const0/const1` from actual dimensions and the user sharpness setting using AMD's CPU-side setup routine.
- Validate linear color range and tone-map order. Do not silently run on gamma-encoded data.
- Compile and validate FP32 first. Add FP16 only after checking format and shader support on the target Android GPU.
- Vulkan and OpenGL ES are separate targets. The upstream Vulkan sample is GLSL 450, not GLSL ES; GLES support is not claimed.
- The shader must include the exact matching `ffx_a.h` and `ffx_cas.h` files and preserve the AMD license notices.

## Important status

Source is now present in this research repository. It has **not** been compiled for Bright Memory, registered as a UE4 global shader, inserted into the game's shader library, connected to an RDG pass, or packaged into an APK. Those steps require the exact UE4 Android shader platform and native renderer integration. No runtime support is claimed yet.
