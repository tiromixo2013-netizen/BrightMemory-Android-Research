# Bright Memory free build lab

This branch isolates the restore and analysis environment from the private repository so Actions runs use the public-repository standard runner allowance.

## Workflow
- `.github/workflows/brightmemory-free-build-lab.yml`
- Manual trigger only.
- Uses Ubuntu 24.04 standard GitHub-hosted runner.
- Installs OpenJDK 17, Android SDK platform/build-tools, NDK 25.2.9519653, CMake, Ninja, apktool, LLVM/Clang, binutils and rclone.
- Downloads the six archived source parts from MEGA, verifies each archive SHA-256, tests each ZIP, reassembles the APK, then verifies the original APK SHA-256 and size.
- Extracts only the ARM64 `libUE4.so` for ELF and string inspection; apktool decodes resources/manifest.
- Saves the verified APK and reports back to MEGA. It does not upload multi-gigabyte GitHub Actions artifacts or commit binaries to Git.

## Required repository secrets
The public repository must already have `MEGA_EMAIL` and `MEGA_PASSWORD`; `MEGA_2FA` is optional. These are the same names used by the successful public-to-MEGA backup workflow. Never print secret values in logs.

## Failure handling
1. **MEGA authentication/listing failure:** check the secret names and MEGA session/2FA; do not change the source archives. Test remote listing first.
2. **Missing or partial object:** rerun only after checking the exact remote filename and byte size; rclone retries are enabled.
3. **Archive checksum failure:** stop immediately; do not reassemble or use the bad part. Compare the remote object against the six pinned SHA-256 values in the workflow.
4. **Disk pressure:** the runner cleans large preinstalled SDK/.NET/CodeQL directories first. If disk still runs low, avoid full APK extraction; retain streaming `unzip -p` for individual native libraries and remove decoded temporary files before further analysis.
5. **APK checksum mismatch:** stop; preserve all six original archives and report the measured size/hash. Never patch or rebuild a mismatched input.
6. **Toolchain install failure:** inspect the failed SDK/package step and pin the working package version; do not silently skip the Android SDK/NDK setup.
7. **Build stage:** this first workflow deliberately restores and inventories the exact APK. A real UE4.27.2 engine build needs the matching engine source tree, project/plugin source, generated headers, Android toolchain compatibility and signing configuration. Do not claim a modded APK until those are present and a full build plus device/runtime test succeeds.

## Scope
The APK is an input package, not a complete editable Unreal project. The next phase is to identify the exact engine/project/plugin sources and construct the agreed native post-processing dispatcher in source. Do not commit the APK or proprietary game assets to this public repository.
