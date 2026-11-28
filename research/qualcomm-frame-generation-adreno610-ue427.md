# Qualcomm frame-generation compatibility — Bright Memory v1.34

## Target
- Samsung Galaxy M14 4G
- Qualcomm Adreno 610
- Android 15, arm64-v8a
- Bright Memory: Infinite Android v1.34, UE4.27.2
- The inspected game profile uses Android OpenGL ES; Vulkan-only integrations are not assumed available.

## Decision
No Qualcomm frame-generation SDK has been verified as compatible with this exact Adreno 610 + UE4.27.2 + OpenGL ES target. Keep Frame Generation set to Off and keep x2/x3/x4 unavailable until a real backend is proven.

## Qualcomm candidates checked

### Adreno Neural Fusion (ANF)
ANF is a genuine Qualcomm frame-generation SDK. It synthesizes an intermediate scene frame from consecutive color frames, depth, and motion vectors. Its public native requirements currently specify Snapdragon 8 Elite Gen 6 or newer and Vulkan. Qualcomm's Unreal Engine plugin branch targets UE5.0–UE5.8, not UE4.27. Therefore it is not a drop-in option for this game or the Adreno 610 target.

### Older Adreno Frame Motion Engine / AFME
Older Qualcomm frame extrapolation and motion-estimation facilities exist in Adreno tooling, but the user explicitly excluded AFME. Do not silently substitute it or relabel it as another algorithm.

## Required acceptance checks for any future Qualcomm candidate
1. Public SDK/API or source must explicitly support Adreno 6xx, not just ARM64.
2. Must support the game's active Android RHI (OpenGL ES unless a verified Vulkan build is produced).
3. Must be integrable with UE4.27.2 and the game's actual renderer.
4. Must synthesize distinct intermediate scene frames using valid motion data or a documented estimation path.
5. Must preserve UI composition and handle disocclusion/occlusion artifacts.
6. Must include a presentation schedule and be profiled on Adreno 610 for GPU cost, latency, stability, and image artifacts.
7. x3/x4 are not to be exposed merely because the UI enum contains them; only expose multipliers the backend genuinely implements.

## References
- Qualcomm Adreno Neural Fusion SDK: https://github.com/SnapdragonGameStudios/adreno-neural-fusion
- Qualcomm ANF integration guide: https://github.com/SnapdragonGameStudios/adreno-neural-fusion/blob/main/INTEGRATION-GUIDE.md
- Qualcomm UE plugin version matrix: https://github.com/SnapdragonGameStudios/snapdragon-game-plugins-for-unreal-engine

This is a compatibility research note, not evidence of a compiled or device-tested implementation.
