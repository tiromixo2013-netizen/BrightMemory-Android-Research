# Bright Memory Android APK split artifact index

Created: 2026-11-26
Purpose: preserve the exact GitHub Actions artifact references for the six split APK-package archives so they can be reacquired while GitHub retains them.

## Source workflow run

- Repository: `tiromixo2013-netizen/BrightMemory-Android-Research`
- Workflow run: [45728055093](https://github.com/tiromixo2013-netizen/BrightMemory-Android-Research/actions/runs/45728055093)
- Source branch at run time: `main`
- Source commit: `301488b64973c1dfa2646a5e76214b4377dbf40f`
- Artifact count: 6
- Combined archive size (artifact ZIPs): 2,368,584,715 bytes

## Split artifacts (concatenate/extract in numeric order 00 → 05)

| Part | Artifact ID | Size (bytes) | SHA-256 of artifact ZIP | Expires (UTC) |
|---|---:|---:|---|---|
| 00 | 15074908510 | 413778108 | 1bb049386561d55f5f3119e420ff16cd5fb405890e053950ac1840bc5325037e | 2026-11-29T00:21:13Z |
| 01 | 15074759168 | 373939258 | fd3d372bb7a3f2bf80c70f42451afbb0b9f857f244f6cdf2d0cec723d1a2f4e9 | 2026-11-29T00:21:27Z |
| 02 | 15076065137 | 407571086 | fd60c05a9211f9f2b1d2e7bc5ea3df4b49521b473888dbc55171820824dea9de | 2026-11-29T00:21:39Z |
| 03 | 15075611297 | 419545892 | 5615bb1c9a8dcb88463d556079d2fdbb3d0c06b0d94112aac44630d06054b889 | 2026-11-29T00:21:52Z |
| 04 | 15075336701 | 419550110 | f856fb69e2b5349793d5bfda7d1b37472f80ada5b0fefaee4909690fb36fad55 | 2026-11-29T00:22:04Z |
| 05 | 15075441932 | 334200261 | a4eb149cf09a095b4fd61f0de3b5db91097b12ab69d53d713fd67c8f614b7ce1 | 2026-11-29T00:22:17Z |

## Important retention warning

These are GitHub Actions artifacts, not permanent repository files. GitHub reported all six as unexpired at index creation, but their listed expiration is 2026-11-29 (UTC). This index preserves their IDs, checksums, sizes, order, and source run; it does not extend GitHub's artifact retention.

## Why the APK bytes are not committed here

This repository is public and the package contains proprietary copyrighted game binaries/assets. To avoid publishing or redistributing the game package, this branch stores only provenance and retrieval metadata—not APK/split ZIP contents.

If the original artifacts expire, regenerate them from the original legally obtained local package using the repository workflow, or store the package in a private location where you have the right to retain it.

## Secondary inspection artifact

- Run: [45730896386](https://github.com/tiromixo2013-netizen/BrightMemory-Android-Research/actions/runs/45730896386)
- Artifact: `brightmemory-v1.34-inspection`
- Artifact ID: `15076428483`
- Size: 400 bytes
- SHA-256: `d0b435cdf54cba7b99387832f38bf82612bd065b12b6f20099bb5220d938313f`
- Expires: 2026-12-03T00:45:08Z
