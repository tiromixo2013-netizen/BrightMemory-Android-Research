# Qualcomm AFME 1.0 integration status

## Confirmed upstream behavior

The Qualcomm SnapdragonGameStudios sample `samples/amfe_power_saving` uses the OpenGL ES extension
`GL_QCOM_frame_extrapolation` and resolves `glExtrapolateTex2DQCOM` with `eglGetProcAddress`.
It stores two traditionally rendered color buffers, then alternates between a traditional frame and
an extrapolated frame generated with a scale factor of `0.5f`. This is an x2 presentation pattern.

Upstream source:
- https://github.com/SnapdragonGameStudios/adreno-gpu-opengl-es-code-sample-framework/tree/main/samples/amfe_power_saving
- https://github.com/SnapdragonGameStudios/adreno-gpu-opengl-es-code-sample-framework/blob/main/samples/amfe_power_saving/src/Scene.cpp

## Bright Memory experimental controls

- `Off`: leaves the game's normal presentation path unchanged.
- `x2`: Qualcomm AFME only; must remain unavailable until the active GLES context reports the extension,
  the entry point resolves, the renderer captures two real scene frames, and the game owns a safe
  interpolation/presentation schedule including UI composition.
- `x3` and `x4`: not supported by the original AFME 1.0 interface and intentionally rejected.

The Blueprint API exposes `SetFrameGeneration(0|1)`. The runtime command is fail-closed and requires
`bmi.FrameGeneration.AFMEAvailable=1`, a capability flag that must only be registered by a completed
renderer backend. This repository does not yet register that backend. Do not set this flag manually.

## Device and engine validation still required

The Qualcomm sample's existence and its OpenGL ES API do not prove that Samsung Galaxy M14 4G /
Adreno 610 exposes the extension. Device-side extension enumeration is still required. The sample's
own README reports measurements on a Snapdragon 8350; it does not establish Adreno 610 support.

The sample is a standalone GLES application. Integrating it into UE4.27 requires renderer-owned
scene-color capture, synchronization, output texture lifetime management, correct UI composition,
and presentation timing. Merely calling the extension from an unrelated GL context is invalid.

## Other experimental settings

The minimal settings profile keeps Resolution Controller, Graphics Quality, SGSR1, Upscale Sharpness,
the five SGSR1 presets, AMD CAS and CAS Sharpness, Low Latency, and Frame Generation. The existing
Blueprint settings library provides callable entry points for the game UMG layer. The actual shipped
UMG settings screen has not yet been edited or rebuilt, so this is not yet a user-installable menu/APK.

No UE4.27 engine compile, Android shader compile, APK build, or Adreno 610 runtime test is claimed.
