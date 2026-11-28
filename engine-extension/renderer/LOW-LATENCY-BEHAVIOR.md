# Low Latency behavior on the Android OpenGL ES target

- **Off** sets `r.GTSyncType=0` and `r.OneFrameThreadLag=1`, restoring UE4's
  default Game Thread-to-Render Thread synchronization.
- **On** sets `r.GTSyncType=1` and keeps `r.OneFrameThreadLag=1`. Epic documents
  mode 1 as synchronizing the Game Thread to the RHI Thread, the low-latency
  synchronization path intended to prevent the Game Thread running too far
  ahead of presentation. The runtime verifies that both CVars exist and retain
  the requested values before reporting the mode as applied.
- The previous implementation used `r.OneFrameThreadLag=0` alone. That is a
  more aggressive thread-lead change, not the documented UE4.27 low-latency
  sync mode, and may reduce throughput. It is no longer used for the On preset.
- **Ultra (Zero Queue)** remains unavailable. UE4.27 can use Google's Swappy
  frame-pacing library on Android, including a non-pipelined mode that may
  reduce input-to-screen latency, but the Bright Memory v1.34 build has not
  yet been proven to initialize Swappy or expose its pipeline controls at
  runtime. Merely finding UE4/Swappy support in documentation is not enough
  to enable Ultra.

Do not fake Ultra with `glFinish` (which can stall the GPU/CPU), a generic
`r.RHICmdBypass` setting (which does not prove zero driver-queued frames), or
Vulkan-only queue CVars. The inspected Bright Memory v1.34 profile disables
Vulkan and targets OpenGL ES.

This is a thread/presentation scheduling control, not frame generation and not
an FPS booster. Validate On versus Off with touch-to-photon measurements and
frame-time/throughput A/B tests on the target phone. A real zero-queue mode
must also verify pending-frame count and input-to-photon latency.
