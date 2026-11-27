# Verified Bright Memory APK baseline

Date: 2026-11-27

Source: MEGA six-part reassembly job artifact `brightmemory-reassembled-package-report`, run `46020845177`.

## Confirmed package facts

- Six raw split parts were concatenated in numeric order.
- Reassembled size: **2,431,306,902 bytes** (2,319 MiB).
- File signature: Android APK / ZIP, with `AndroidManifest.xml`.
- APK SHA-256: `c35b05ac7b19fd087844de2b6261d33fbe103a6a5714c5e42d6101fab07881e4`.
- ARM64 native engine library exists at `lib/arm64-v8a/libUE4.so`.
- `libUE4.so` uncompressed size in the APK listing: **174,981,736 bytes**.
- Other ARM64 libraries listed: `libc++_shared.so`, `libplaycore.so`, `libRMS.so`, `libtapsdkcore.so`.

## What this proves — and does not prove

This confirms that we have a reassembled APK containing the game's ARM64 Unreal engine binary. It does **not** prove that an upscaler hook, shader, settings UI, or modified APK has been implemented.

The earlier repository README correctly describes the repository as a research scaffold. No game APK or copyrighted game binary is committed to GitHub.

## Immediate engineering sequence

1. Re-download the six MEGA parts in a reproducible workflow and verify the APK SHA-256 above.
2. Extract `libUE4.so` in the CI workspace only; do not upload the game binary as an artifact.
3. Record ELF class, machine, build ID, dependencies, exported/dynamic symbols, and renderer-related strings.
4. Match the binary fingerprint against the selected UE4.27 source tree before using source offsets or ABI assumptions.
5. Only after that implement a minimal, reversible renderer probe. Do not register a custom `ISpatialUpscaler` or patch `FSceneViewFamily` based on guessed offsets.
6. Integrate CAS as a separate post-process pass only after the actual Android RHI and render-graph path is confirmed.

## Next automation

See `.github/workflows/analyze-brightmemory-native.yml`. It rebuilds the package from MEGA, extracts metadata and native diagnostics, and uploads reports only — never the APK or `libUE4.so`.
