# MWSR source provenance

- Upstream: https://github.com/linckosz/moonlight-web/tree/main/mwsr
- Shader: `mwsr/hlsl/mwsr-up.hlsl`
- Upstream blob: `96acdd29f18af91aa6866cc9da5cdf91fb52bb3f`
- License: MIT; retained in `LICENSE`.

The upstream HLSL now has a UE4.27 RDG pixel-pass adapter in `../../BMIMWSRPass.cpp` and a shader wrapper in `../../BMIShaders/Private/BMIMWSR.usf`. The adapter ports the upstream 96-byte constant law and maps `ddx_coarse` / `ddy_coarse` to fragment derivatives for the mobile path. It is wired into BMI registration but has not been compiled against the target engine or Android GLES compiler; output quality and performance still need device validation. Keep the menu option hidden until those checks pass.
