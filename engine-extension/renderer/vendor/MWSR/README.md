# MWSR source provenance

- Upstream: https://github.com/linckosz/moonlight-web/tree/main/mwsr
- Shader: `mwsr/hlsl/mwsr-up.hlsl`
- Upstream blob: `96acdd29f18af91aa6866cc9da5cdf91fb52bb3f`
- License: MIT; retained in `LICENSE`.

This is the upstream HLSL shader, not yet a UE4.27 Mobile Vulkan shader. The upstream file targets Direct3D shader compilation and uses `ddx_coarse` / `ddy_coarse`, a custom 96-byte constant buffer and a linear-clamp sampler. UE4.27 shader parameter binding, mobile derivative behavior, shader compiler compatibility, and the RDG pass remain to be ported and compiled. Do not expose it as an active in-game method yet.
