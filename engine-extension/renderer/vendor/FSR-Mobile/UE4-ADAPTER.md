BMI FSR Mobile shader adapter

The USF wrapper uses the atyuwen optimized FSR1 mobile function with UE4.27 RDG screen-pass bindings. It builds EASU constants per viewport, uses the source's bilinear sample callback plus channel gathers, and passes the user's sharpness value into the fused limiter. This is a source implementation only; target UE shader compilation and GLES 3.x validation are still required.
