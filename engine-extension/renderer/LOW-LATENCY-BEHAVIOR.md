# Low Latency behavior on the Android OpenGL ES target

The menu exposes three labels, but only Off and On are currently selectable.

- **Off** sets `r.OneFrameThreadLag=1`, restoring UE4's normal one-frame Game
  Thread lead. This is the throughput-oriented default.
- **On** sets `r.OneFrameThreadLag=0`, removing that one-frame lead. This can
  reduce input-to-render latency, but can also reduce throughput or make frame
  pacing less consistent on a CPU-limited device.
- **Ultra** is deliberately disabled. The inspected Bright Memory v1.34
  Android profile has Vulkan disabled and targets OpenGL ES. No active GLES
  queue-depth/no-queue control has been verified for this engine build.
  Reusing Vulkan CVars or setting arbitrary queue values would be misleading.

This is a thread scheduling control, not frame generation and not an FPS
booster. Its real effect still needs to be measured on the target phone.
