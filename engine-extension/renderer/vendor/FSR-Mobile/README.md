# FSR Mobile source provenance

- Optimized mobile FSR1 implementation: atyuwen, https://gist.github.com/atyuwen/78d6e810e6d0f7fd4aa6207d416f2eeb
- Source file: `opt_fsr.fxh`; combines EASU and RCAS in one pixel-shader function.
- License: MIT, confirmed by the author in the gist discussion; copyright notice retained in the source.
- AMD shader portability and FSR1 helpers: `ffx_a.h` and `ffx_fsr1.h`, copied from GPUOpen-Effects/FidelityFX-FSR, with `AMD-LICENSE.txt`.

The function expects the host shader to provide the bilinear `FsrEasuSampleH` callback, EASU constants, and the matching shader portability macros. It is source-vendored only: no UE4.27 wrapper shader, RDG pass, or Android OpenGL ES shader compile has been performed. Keep the option hidden until those steps and a visual test pass.


Target-RHI note: Bright Memory v1.34's inspected device profile selects Android OpenGL ES and disables Vulkan. The FSR Mobile helper's HLSL gather/half-precision constructs have not yet been validated by this game's GLES shader compiler; do not register or expose this backend until that compile and an image-output test succeed.
