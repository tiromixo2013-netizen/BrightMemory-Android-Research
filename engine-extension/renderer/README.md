# BMI renderer integration

## Active SGSR1 path

The active minimal profile uses Qualcomm's official UE4.27 SGSR1 plugin as
its only upscaler. The plugin owns the primary spatial-upscale stage through
its `FSGSRViewExtension` and `FSGSRSpatialUpscaler`; the runtime enables
`r.Qualcomm.SGSR.Enabled` and keeps `bmi.Pipeline.Enabled` off. This avoids
two extensions competing for UE4.27's assign-once primary-upscaler slot.

The Qualcomm source is vendored under `vendor/SGSR1/`. The vendored UE4.27
copy already carries a real `Sharpness` RDG parameter, the
`r.Qualcomm.SGSR.Sharpness` CVar, and the 0..1 UI-to-1..2 shader mapping.
Patch `patches/0002-sgsr1-runtime-sharpness.patch` reproduces the equivalent
runtime control when starting from the pristine official Qualcomm branch.
The shared five-level preset sets the render percentage and a recommended
SGSR1 sharpness value; it does not add a second upscaler or render-scale control.

`patches/0001-register-bmi-spatial-upscaler.patch` registers the optional BMI
research dispatcher in the reference Renderer module. It is not required to
activate Qualcomm SGSR1 and remains disabled in the agreed minimal profile.
The dispatcher retains safe built-in Nearest/Bilinear fallbacks and source
adapters for research; MWSR, FSR Mobile and SGSR2 are not exposed as choices
in this profile. The optional BMI extension never overwrites an existing
primary upscaler. If another extension claims the primary slot first, BMI
leaves it untouched.

The official Qualcomm SGSR extension also checks that the primary slot is
empty before assigning it. Do not enable the BMI dispatcher and Qualcomm
SGSR1 as competing primary spatial upscalers.

## Optional research components

The BMI dispatcher includes optional source adapters for MWSR and FSR Mobile,
but they are not part of the active menu and are not claimed to be compiled or
validated on the target. Qualcomm SGSR2 research source is also separate from
the active SGSR1-only path; do not expose it as an alias for SGSR1.

## Optional stages

AMD FidelityFX CAS headers are vendored under `vendor/AMD-CAS/`, but there
is no registered CAS RDG shader pass. The runtime rejects enabling CAS while
the availability CVar is false. Do not replace CAS with
`r.Tonemapper.Sharpen`.

The HUD callback exists in the dispatcher contract, but there is no actual
telemetry compositor registered. Its availability remains false.

## Runtime and Blueprint settings

`Source/BMIGraphicsRuntime` contains the settings command bridge and
Blueprint-callable API. The active menu schema is in
`../../config/minimal-experimental-sgsr1-cas.json` (also mirrored to
`../../config/graphics-settings.json` for the runtime integration entry point). The agreed minimal menu order is Resolution Controller, Graphics Quality,
Render Scale, SGSR1, Upscale Sharpness, shared Upscale Preset, AMD CAS,
CAS Sharpness, Low Latency, Frame Generation Method, and Frame Generation
Multiplier. Frame Pacing, Performance HUD, and alternate upscalers are excluded
from this profile.

The Blueprint library is an API seam, not a completed in-game menu. The v1.34 extraction identifies `BMI_Graphics_UMG_C` inside `BMI_Menu_Seting_UMG_C`, and every agreed setting now has an explicit Blueprint API mapping. See `UMG-RUNTIME-BINDING-MAP.md`. The cooked widget graph itself has not yet been rebound to those APIs; this still requires the matching UE4.27.2 project/editor and an in-game verification. The plugin has not been compiled against the shipped engine snapshot.

## Target constraints and verification

The inspected target profile disables Vulkan and uses Android OpenGL ES. The
public UE4.27 reference confirms the general RDG screen-pass API shape, but
does not prove that the shipped game uses an identical renderer snapshot.
Consequently Vulkan-only frame-pacing CVars are not presented as a working
solution. The active GLES/Swappy pacing path still needs integration. FSR
Mobile remains source-only: its gather callbacks, half-precision path, shader-
directory mapping, and UE shader compiler output must be checked against the
game's exact GLES target before an adapter can be registered. Low Latency On
currently maps to UE4's `r.OneFrameThreadLag=0`; Ultra/No Queue is rejected
because no supported active-RHI queue-depth control is verified.

Run `python scripts/validate_bmi_runtime.py` for source/schema consistency.
That is a static source check only. It does not compile UE shaders, prove
renderer ABI compatibility, produce an APK, or replace an Adreno 610 device
test. No modified APK or on-device success is claimed.
