# Bright Memory: Infinite v1.34 — actual PAK/Oodle extraction result

Date: 2026-11-27  
Workflow run: https://github.com/tiromixo2013-netizen/BrightMemory-Android-Research/actions/runs/46067077848  
Artifact: `brightmemory-v1.34-config-ui-extract-oodle` (artifact ID `15230794214`)  
APK SHA-256 verified by the workflow: `c35b05ac7b19fd087844de2b6261d33fbe103a6a5714c5e42d6101fab07881e4`

## Result

- All six stored APK parts downloaded successfully and were reassembled.
- APK SHA-256 verification and ZIP integrity test passed.
- Both embedded ZIP containers were opened and all nine PAK chunks were found.
- `repak` v0.2.3 was built with its explicit `oodle` feature enabled.
- All nine PAK chunks were processed by the targeted extraction step without an unpack error.
- The artifact contains 817 uploaded files (extracted assets plus reports); the extracted tree is about 43 MiB.
- The workflow completed with conclusion `success`.

This confirms Oodle-backed PAK extraction for the selected files. It does **not** mean every payload in all nine PAKs was fully unpacked: the command deliberately filtered to config, UMG, Settings, and Menu paths.

## Confirmed graphics/settings assets

From `pakchunk0-Android_ASTC`:

- `BMI_Mobile/Content/UISystem_BMI/Menu/MainMenu/Two/BMI_Menu_Seting_UMG.uasset` and `.uexp`
- `BMI_Mobile/Content/UISystem_BMI/Menu/Seting/Graphics/BMI_Graphics_UMG.uasset` and `.uexp`
- `BMI_Mobile/Content/UISystem_BMI/Menu/Seting/Graphics/BMI_Graphics_Screen.uasset` and `.uexp`
- `BMI_Mobile/Content/UISystem_BMI/Menu/Seting/Graphics/BMI_Graphics_Shadows.uasset` and `.uexp`
- `BMI_Mobile/Content/UISystem_BMI/Menu/Seting/Graphics/BMI_Graphics_SSAO.uasset` and `.uexp`
- `BMI_Mobile/Content/UISystem_BMI/Menu/Seting/Graphics/BMI_Graphics_FPSLimb_Android.uasset` and `.uexp`
- `BMI_Mobile/Content/UISystem_BMI/Menu/Seting/Graphics/BMI_GamePlay_AdvGraphics_TexBox.uasset` and `.uexp`

## Binary-string evidence in the extracted assets

- The parent settings widget contains the Graphics button event and references `BMI_Graphics_UMG`.
- `BMI_Graphics_Screen.uexp` contains `r.MobileContentScaleFactor` values `0.75`, `0.85`, `1`, `1.2`, `1.35`, `1.5`, and `0`, with labels `540P`, `640P`, `720P`, `900P`, `980P`, `1080P`, and `100%`.
- `BMI_Graphics_Shadows.uexp` contains `r.ShadowQuality 0` and `r.ShadowQuality 1`.
- `BMI_Graphics_SSAO.uexp` contains `r.Mobile.AmbientOcclusionQuality 0` and `r.Mobile.AmbientOcclusionQuality 1`.
- `BMI_Graphics_FPSLimb_Android.uexp` contains `t.maxfps` values `45`, `60`, `90`, and `120`.

These strings establish that the extracted cooked assets reference those settings commands and labels. They do not, by themselves, prove runtime behavior or reveal every Blueprint execution branch.

## Next step

Parse the extracted cooked `.uasset/.uexp` pairs into a readable WidgetTree/property report using a UE4.27-compatible asset parser. Inspect the event graph/property references before attempting any asset edits. The current extraction is evidence for analysis, not a modified game build.
