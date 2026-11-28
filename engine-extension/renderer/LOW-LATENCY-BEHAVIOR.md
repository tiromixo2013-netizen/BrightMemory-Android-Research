# Low Latency behavior on the Android OpenGL ES target

- **Off** sets `r.OneFrameThreadLag=1`, restoring UE4's normal one-frame Game
  Thread lead. This is the throughput-oriented default.
- **On** sets `r.OneFrameThreadLag=0`, removing that one-frame lead. This can
  reduce input-to-render latency, but can also reduce throughput or make frame
  pacing less consistent on a CPU-limited device.
- **Ultra (Zero Queue)** is the requested strict mode: zero pending frames in
  the active Android GLES/EGL presentation path. It is not equivalent to
  disabling UE's Game Thread lead. It remains disabled until the actual swap
  and driver queue can be controlled and measured on this target.

Do not fake Ultra with `glFinish` (which can stall the GPU/CPU), a generic
`r.RHICmdBypass` setting (which does not prove zero driver-queued frames), or
Vulkan-only queue CVars. The inspected Bright Memory v1.34 profile disables
Vulkan and targets OpenGL ES.

This is a thread/presentation scheduling control, not frame generation and not
an FPS booster. A real zero-queue implementation must verify pending-frame
count and input-to-photon latency on the target phone.
