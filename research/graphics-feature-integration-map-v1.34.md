# Bright Memory v1.34 — practical integration map for the agreed graphics features

Date: 2026-11-27  
Target: Android v1.34 ARM64 only  
Verified source APK SHA-256: `c35b05ac7b19fd087844de2b6261d33fbe103a6a5714c5e42d6101fab07881e4`

## New finding: the original game already has a graphics settings UMG

The prior targeted extraction did include the real graphics menu assets. The important files are:

- `BMI_Mobile/Content/UISystem_BMI/Menu/Seting/Graphics/BMI_Graphics_UMG.uasset/.uexp`
- `BMI_Graphics_Screen.uasset/.uexp`
- `BMI_Graphics_Shadows.uasset/.uexp`
- `BMI_Graphics_SSAO.uasset/.uexp`
- `BMI_Graphics_FPSLimb_Android.uasset/.uexp`
- `BMI_GamePlay_AdvGraphics_TexBox.uasset/.uexp`
- Parent menu: `UISystem_BMI/Menu/MainMenu/Two/BMI_Menu_Seting_UMG.uasset/.uexp`

Binary string inspection of the serialized exports found existing command strings:

### Existing resolution / render scale control

`BMI_Graphics_Screen.uexp` contains:

- `r.MobileContentScaleFactor 0.75`
- `r.MobileContentScaleFactor 0.85`
- `r.MobileContentScaleFactor 1`
- `r.MobileContentScaleFactor 1.2`
- `r.MobileContentScaleFactor 1.35`
- `r.MobileContentScaleFactor 1.5`
- `r.MobileContentScaleFactor 0`
- Labels including 540P, 640P, 720P, 900P, 980P, 1080P, and 100%.

This is strong evidence that the game's existing display UI applies mobile content scale presets through Unreal console-variable commands. It is not yet proof that every command maps to the same physical pixel dimensions on all aspect ratios or that the command is safe to change during active rendering.

### Existing shadow and ambient-occlusion controls

`BMI_Graphics_Shadows.uexp` contains:

- `r.ShadowQuality 0`
- `r.ShadowQuality 1`

`BMI_Graphics_SSAO.uexp` contains:

- `r.Mobile.AmbientOcclusionQuality 0`
- `r.Mobile.AmbientOcclusionQuality 1`

Thus the shipped graphics menu already has real engine CVar hooks for at least shadow and mobile AO quality. This is a much better integration point than inventing an unrelated settings screen.

### Existing frame-rate selection

`BMI_Graphics_FPSLimb_Android.uexp` contains:

- `t.maxfps 45`
- `t.maxfps 60`
- `t.maxfps 90`
- `t.maxfps 120`

The packaged engine defaults also include smooth-frame-rate bounds and a 60 FPS frame-rate lock. The runtime effect and device-supported values must be tested; merely seeing command strings is not proof that 90/120 FPS works.

## Proposed integration architecture

The safest architecture is a two-layer extension:

1. **Preserve the game's existing UMG graphics menu and settings persistence.** Reuse its Screen / Shadows / SSAO / FPS setting patterns and its existing `UserSave` references. Do not replace unrelated audio, controls, gameplay, or localization behavior.
2. **Add a native ARM64 graphics extension inside the Android process.** It owns the new controls and calls engine APIs/CVars through verified runtime interfaces. If the original UMG binary can be safely extended with the available tools, expose the new controls there; otherwise first validate a native in-game panel before attempting destructive edits to cooked UAssets.

The existing UMG exports are cooked binary assets. Editing arbitrary bytes or appending guessed UMG objects is not a safe method. We need a compatible UE4.27 asset parser/editor or a validated runtime widget injection route to modify the original menu.

## Feature-by-feature implementation map

| Agreed feature | Real integration point | Status / limitation |
|---|---|---|
| Resolution Controller | Existing Screen UMG + device profile + native display metrics | Existing scale presets confirmed. Add Native option only after mapping device pixels, aspect ratio, orientation and DPI correctly. |
| Graphics Quality: Low → Extreme | Map one preset to actual scalability categories and supported mobile CVars | Engine exposes UGameUserSettings scalability functions. Existing menu evidence confirms only shadow/AO controls so far. Each category needs visual/runtime validation. |
| Render Scale | Extend existing `r.MobileContentScaleFactor` path or use validated screen-percentage API | Best first feature. Must distinguish render resolution from Android system DPI and avoid reallocating render targets every frame. |
| Upscaling method selector | Native render graph / mobile post-processing integration | UI alone is insufficient. Each method must own a real render pass and resource lifecycle. |
| SGSR1 | Integrate a compatible Qualcomm implementation only if the license/source/API and GPU requirements are available | Not present in inspected config. Device/driver support must be verified. |
| SGSR2 | Same; do not label a different shader as SGSR2 | No integration found yet. |
| MWSR | Requires a defined implementation/source and real mobile render pass | No implementation identified in the APK. |
| FSR Lite | Implement a real spatial upscaler pass; document exact algorithm/version | Not present in inspected config. Do not imply FSR 2.x. |
| Nearest / Bilinear | Simple real spatial sampling passes | Most practical first upscaler implementations after render-pass injection is proven. |
| Upscale Sharpness + unified preset | Feed parameters to the selected upscaler; preset translates to that method's scale/sharpness parameters | One shared preset control, method-specific mappings. |
| AMD CAS | Integrate FidelityFX CAS shader/pass after upscaling or at native resolution | No CAS integration confirmed in package. Must validate shader precision and mobile GPU cost. |
| Frame Generation x2/x3/x4 | Requires real intermediate-frame synthesis, valid motion vectors/depth or robust reconstruction, synchronization and pacing | No verified game motion-vector feed or existing FG found. Treat as blocked until render data is proven. Never fake by repeating frames. |
| Frame Pacing | Present queue / swap interval / frame-time controller | Must measure actual frame-time variance and input latency; avoid fighting Unreal's own smoother. |
| Low Latency | Android/driver/UE render queue control with safe queue depth | Do not force zero queue blindly. Expose Ultra only when measured stable. |
| Performance Overlay | Native lightweight telemetry and frame-time sampling | CPU/GPU/temp availability differs by device; unsupported values show N/A. Sampling must be throttled and low overhead. |

## Exact implementation sequence

### Phase 1 — safe, existing engine controls
1. Build a runtime probe for the current graphics CVar values and verify command execution.
2. Add Native display detection and a mapping table for the existing Screen control.
3. Extend Render Scale while preserving the original Screen menu semantics.
4. Wire Graphics Quality to actual supported scalability categories; confirm shadow/AO changes visibly.
5. Add a lightweight settings persistence layer that does not overwrite unrelated `UserSave` data.

### Phase 2 — native rendering features
6. Establish a safe mobile post-process/render-pass insertion point.
7. Implement Bilinear and Nearest first as control passes to validate source/destination textures, viewport, formats, and synchronization.
8. Add FSR Lite / SGSR-family / MWSR only when their exact implementations and compatibility are available.
9. Add CAS after the selected upscaler, with a sharpness control and unified preset mapping.

### Phase 3 — timing and telemetry
10. Add frame pacing and low-latency controls, then measure frame-time percentiles and input latency.
11. Add overlay with FPS, average, 1% low, frame time, CPU/GPU/temperature where available.

### Phase 4 — frame generation research gate
12. Trace whether the game's motion vectors, depth, camera transforms and frame history are accessible at the mobile render stage.
13. Only if a complete interpolation path is demonstrated, prototype x2, then x3/x4. Otherwise mark FG unsupported rather than shipping a non-functional toggle.

## Current blocker

We have the original cooked UMG assets and can identify their existing CVar strings, but we do not have the game's Unreal project source or a confirmed compatible UMG editor pipeline. The engine is a stripped shipping binary. The next engineering task is to inspect the UMG serialized exports and trace the native CVar execution path, then build a minimal ARM64 probe that can read/apply one existing graphics setting without changing the game package.

No modified APK has been produced yet. This document is an evidence-based integration plan, not a claim that any new technique is already implemented.
