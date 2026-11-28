# GLES Motion-Vector Reprojection (experimental)

This is an independent, portable OpenGL ES 3.0 fragment-shader prototype for the
Bright Memory: Infinite UE4.27 Android research branch. It is separate from
Qualcomm AFME and does not call any QCOM extension.

## Inputs and convention

- `u_CurrentFrame`: most recently rendered scene-color image.
- `u_PreviousFrame`: prior genuine scene-color image.
- `u_MotionVectors`: RG motion texture in UV units. The convention used by
  this prototype is **previousUV - currentUV**, sampled at current-frame UV.
- `u_Phase`: forward prediction phase. For x2 render one generated frame at
  0.5; for x3 render at 1/3 and 2/3; for x4 render at 1/4, 1/2 and 3/4.
- `u_MotionScale`: conversion factor if the engine velocity texture is not
  already expressed in UV units.
- `u_ValidationThreshold`: scene-color validation threshold.

The pass predicts by warping the current image opposite the current-to-previous
motion vector. It samples the previous image for a cheap confidence heuristic
and falls back toward the current image when validation is poor.

## Important limitations

This is a **shader prototype**, not a complete frame-generation backend. It
does not create motion vectors, solve disocclusion, handle camera cuts, infer
object motion when UE velocity is missing, manage history textures, schedule
presentation, or composite UI. The simple color-difference confidence test can
reject valid motion under lighting changes and cannot reliably detect all
occlusions. It must not be exposed as working until those renderer tasks and
device tests are complete.

The shader is intentionally GLES 3.0 compatible and uses no vendor-specific
extension. Actual performance depends on texture formats, bandwidth, GPU
driver, and the renderer's motion-vector source. No Adreno or Mali performance
claim is made.

## Multiplier mapping

- x2: one generated image at phase 0.5.
- x3: two generated images at phases 1/3 and 2/3.
- x4: three generated images at phases 1/4, 2/4 and 3/4.

The phases are not equivalent to simply changing one uniform and presenting
duplicate frames: each output requires a correctly timed presentation slot.
The game renderer must retain two genuine source frames and schedule all
generated outputs between source-frame presentations.

## UE4.27 integration requirements

1. Obtain the actual velocity texture and document its direction, scale,
   jitter convention, and coordinate space for the target build.
2. Add a renderer-owned pass with RDG/resource lifetime and GLES RHI support.
3. Add disocclusion handling (at minimum depth/validity rejection and
   conservative hole filling) and camera-cut/history invalidation.
4. Keep HUD/UI composition on the correct side of interpolation.
5. Implement pacing/presentation and fail-closed capability flags per
   multiplier. Never enable a multiplier just because the shader compiles.
6. Compile the target UE4.27 Android GLES shader permutation and validate on
   Adreno 610 and at least one Mali device.

Qualcomm AFME remains a separate x2-only candidate, gated by the actual
`GL_QCOM_frame_extrapolation` extension and its UE renderer adapter.
