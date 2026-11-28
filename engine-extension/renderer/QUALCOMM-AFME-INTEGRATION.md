# Frame Generation integration and capability gates

## Requested modes

The settings/API exposes Off, x2, x3 and x4.

- x2: Qualcomm AFME 1.0 (GL_QCOM_frame_extrapolation) is the candidate. Its public sample demonstrates one extrapolated image between two rendered source frames. This does not prove support on Adreno 610 or integration in this game.
- x3 / x4: the original Qualcomm AFME 1.0 interface does not implement these multipliers. They require a separate multi-frame interpolation backend with motion/depth handling and renderer-owned presentation scheduling. They must not be faked by repeating or blending frames.

## Fail-closed runtime behavior

The runtime command accepts 0..3, but a mode activates only if its backend registers the corresponding availability CVar:

- x2: bmi.FrameGeneration.AFMEAvailable=1
- x3: bmi.FrameGeneration.X3Available=1
- x4: bmi.FrameGeneration.X4Available=1

No availability CVar is registered by this source-only integration yet. Therefore all non-Off modes remain safely unavailable until their complete backend is compiled and validated. Do not set these flags manually.

## Backend completion criteria

Each backend must verify device/API capability, capture genuine game frames, preserve motion/depth/history as required, synchronize GPU resources, composite UI correctly, schedule presentation at the requested multiplier, and validate latency/ghosting/tearing in gameplay. x3/x4 are not AFME features.

## Current target limitations

Target: Bright Memory: Infinite Android v1.34, UE4.27.2, Samsung Galaxy M14 4G / Adreno 610. AFME extension availability on this driver remains unverified. No UE4.27 engine compile, Android shader compile, APK build, or device runtime validation is claimed.
