import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ENGINE = ROOT / "engine-extension"
RENDERER = ENGINE / "renderer"
VENDOR = RENDERER / "vendor"


def read(path):
    return path.read_text(encoding="utf-8")


plugin = json.loads(read(ENGINE / "BMIGraphicsRuntime.uplugin"))
assert plugin["FileVersion"] == 3
assert any(m["Name"] == "BMIGraphicsRuntime" and m["Type"] == "Runtime"
           for m in plugin["Modules"])

runtime = read(ENGINE / "Source/BMIGraphicsRuntime/Private/BMIGraphicsRuntimeModule.cpp")
blueprint_header = read(ENGINE / "Source/BMIGraphicsRuntime/Public/BMIGraphicsSettingsLibrary.h")
blueprint_source = read(ENGINE / "Source/BMIGraphicsRuntime/Private/BMIGraphicsSettingsLibrary.cpp")
for token in [
    "SetResolutionController", "r.MobileContentScaleFactor", "GetDesktopResolution()", "TargetHeight[] = { ShortSide, 540, 640, 720, 900, 980, 1080 }", "AppliedScale = ContentScale->GetFloat()", "display/DPI unchanged.",
    "SetOverallScalabilityLevel", "SetViewDistanceQuality", "SetShadowQuality", "SetAntiAliasingQuality", "SetTextureQuality", "SetVisualEffectQuality", "SetFoliageQuality", "SetShadingQuality", "SetPostProcessingQuality", "SetUpscalePreset",
    "bmi.Pipeline.Enabled", "SetUpscaleSharpness",
    "bmi.Pipeline.CASAvailable", "SetCAS", "SetCASSharpness",
    "SetFramePacing", "r.OneFrameThreadLag", "SetPerformanceHUD",
    "bmi.Pipeline.HUDAvailable", "SetFrameRateLimit", "SetFrameGeneration", "bmi.FrameGeneration.AFMEAvailable", "bmi.FrameGeneration.GLESX2Available", "bmi.FrameGeneration.GLESX3Available", "bmi.FrameGeneration.GLESX4Available",
]:
    assert token in runtime, f"Missing runtime bridge symbol: {token}"
assert "static bool ApplyRenderPercentage(float Percent, const TCHAR* Source)" in runtime
assert "static bool EnsureSGSR1Enabled()" in runtime
assert "ApplyRenderPercentage(RenderScale[Preset], TEXT(\"Upscale Preset\"))" in runtime
assert "SetCVarFloatAndVerify(TEXT(\"r.Qualcomm.SGSR.Sharpness\"), EdgeSharpness[Preset])" in runtime
assert "this profile supports Qualcomm SGSR1 only (index 0)" in runtime
assert "if (Method != 0) return false;" in blueprint_source
assert "Method < 0 || Method > 5" not in blueprint_source
assert "FDefaultValueHelper::ParseFloat(Args[0], Percent)" in runtime
assert "Percent < 50.0f || Percent > 150.0f" in runtime
assert "SetResolutionScaleValueEx(Percent)" in runtime
assert "GetResolutionScaleInformationEx(Normalized, Applied, Minimum, Maximum)" in runtime
assert "Resolution Controller and Android display/DPI remain unchanged." in runtime
assert "Ultra (Zero Queue) is unavailable" in runtime or "Ultra (No Queue) is unavailable" in runtime
for method in ["SetViewDistanceQuality", "SetShadowQuality", "SetAntiAliasingQuality", "SetTextureQuality", "SetVisualEffectQuality", "SetFoliageQuality", "SetShadingQuality", "SetPostProcessingQuality"]:
    assert f"Settings->{method}(Group)" in runtime, f"Graphics quality category not applied: {method}"
assert "const int32 GroupQuality[] = { 0, 0, 1, 2, 3, 4, 4 };" in runtime
assert "0 Very Low, 1 Low, 2 Medium, 3 High, 4 Very High, 5 Ultra, 6 Extreme" in runtime
assert "const float ViewDistance[] = { 0.50f, 0.65f, 0.80f, 1.00f, 1.10f, 1.20f, 1.35f };" in runtime
assert "const int32 Shadows[] = { 0, 0, 1, 2, 3, 4, 5 };" in runtime
assert "const int32 MobileAO[] = { 0, 0, 0, 1, 1, 2, 3 };" in runtime
assert "FDefaultValueHelper::ParseInt(Args[0], Quality)" in runtime
assert "if (Quality < 0 || Quality > 6)" in runtime
assert "FDefaultValueHelper::ParseInt(Args[0], Choice)" in runtime
assert "FDefaultValueHelper::ParseInt(Args[0], Requested)" in runtime
assert "IConsoleManager" in runtime and "FAutoConsoleCommand" in runtime, "Runtime console command bridge is missing"
assert 'FindCVar(TEXT("r.Qualcomm.SGSR.Enabled"))' in runtime
assert "active Android OpenGL ES pacer/Swappy path has not yet been integrated" in runtime

blueprint_header = read(ENGINE / "Source/BMIGraphicsRuntime/Public/BMIGraphicsSettingsLibrary.h")
blueprint_source = read(ENGINE / "Source/BMIGraphicsRuntime/Private/BMIGraphicsSettingsLibrary.cpp")
assert "QualityLevel > 6" in blueprint_source, "Blueprint API does not accept Extreme tier"
for method in [
    "SetResolutionController", "SetGraphicsQuality", "SetRenderScale",
    "SetUpscaler", "SetUpscaleSharpness", "SetUpscalePreset", "SetAMDCAS",
    "SetCASSharpness", "SetFramePacing", "SetLowLatency",
    "SetPerformanceHUD", "SetFrameLimit", "SetFrameGeneration", "SetFrameGenerationMethod", "SetFrameGenerationMultiplier",
]:
    assert f"static bool {method}(" in blueprint_header, f"Missing Blueprint API: {method}"
    assert f"UBMIGraphicsSettingsLibrary::{method}(" in blueprint_source, f"Missing implementation: {method}"

minimal = json.loads(read(ROOT / "config/minimal-experimental-sgsr1-cas.json"))
assert next(item for item in minimal["settings"] if item["id"] == "graphics_quality")["options"] == ["Very Low", "Low", "Medium", "High", "Very High", "Ultra", "Extreme"]
assert minimal["menu_order"] == [
    "resolution_controller", "graphics_quality", "render_scale", "upscaler",
    "upscale_sharpness", "upscale_preset", "amd_cas", "cas_sharpness",
    "low_latency", "frame_generation_method", "frame_generation_multiplier",
]
fg_method = next(item for item in minimal["settings"] if item["id"] == "frame_generation_method")
fg_multiplier = next(item for item in minimal["settings"] if item["id"] == "frame_generation_multiplier")
assert fg_method["options"] == ["Off", "Qualcomm AFME", "GLES Motion Reprojection"]
assert fg_multiplier["options"] == ["x2", "x3", "x4"]
assert "x3" in runtime and "x4" in runtime
assert "Mode > 3" in runtime
assert "Mode > 3" in blueprint_source
gles_fg_shader = read(RENDERER / "BMIShaders/Private/BMIGLESFrameReprojection.frag")
for token in ["#version 300 es", "u_CurrentFrame", "u_PreviousFrame",
              "u_MotionVectors", "u_Phase", "u_ValidationThreshold"]:
    assert token in gles_fg_shader, f"GLES reprojection shader missing: {token}"
gles_fg_doc = read(RENDERER / "GLES-FRAME-REPROJECTION.md")
assert "x2" in gles_fg_doc and "x3" in gles_fg_doc and "x4" in gles_fg_doc
assert "not a complete frame-generation backend" in gles_fg_doc
framegen_cpp = read(RENDERER / "BMIFrameGeneration.cpp")
framegen_header = read(RENDERER / "BMIFrameGeneration.h")
for token in ["SetBackendCapabilities", "IsBackendAvailable", "MultiplierX2",
              "MultiplierX3", "MultiplierX4", "QualcommAFME", "GLESReprojection"]:
    assert token in framegen_cpp + framegen_header, f"Frame generation gate missing: {token}"
assert "CVarAFMECapabilities" in framegen_cpp
for token in ["GLESX2Available", "GLESX3Available", "GLESX4Available"]:
    assert token in framegen_cpp
runtime_fg = runtime[runtime.index("static bool IsFrameGenerationAvailable"):runtime.index("static FAutoConsoleCommand CmdFrameGeneration")]
for token in ["SetFrameGenerationMethod", "SetFrameGenerationMultiplier",
              "GLESX2Available", "GLESX3Available", "GLESX4Available"]:
    assert token in runtime_fg, f"Runtime frame generation selector missing: {token}"
bp_header = read(ENGINE / "Source/BMIGraphicsRuntime/Public/BMIGraphicsSettingsLibrary.h")
bp_cpp = read(ENGINE / "Source/BMIGraphicsRuntime/Private/BMIGraphicsSettingsLibrary.cpp")
assert "SetFrameGenerationMethod" in bp_header and "SetFrameGenerationMethod" in bp_cpp
assert "SetFrameGenerationMultiplier" in bp_header and "SetFrameGenerationMultiplier" in bp_cpp
assert "AFME is x2-only" in runtime
assert "GLES backend only when that exact multiplier is registered" in runtime


config = json.loads(read(ROOT / "config/graphics-settings.json"))
assert config["menu_order"] == minimal["menu_order"], "Runtime settings schema differs from the agreed minimal menu"
assert config["settings"] == minimal["settings"], "Runtime settings schema differs from the minimal experimental profile"
assert config.get("ui_integration", {}).get("target_widget_class") == "BMI_Graphics_UMG_C"
for item, api in zip(minimal["settings"], ["SetResolutionController","SetGraphicsQuality","SetRenderScale","SetUpscaler","SetUpscaleSharpness","SetUpscalePreset","SetAMDCAS","SetCASSharpness","SetLowLatency","SetFrameGenerationMethod","SetFrameGenerationMultiplier"]):
    assert item.get("blueprint_api", "").endswith("::" + api), f"Missing Blueprint binding for {item['id']}"
assert "cooked widget graph has not been edited or rebound" in minimal["ui_integration"]["status"]
umg_map = read(RENDERER / "UMG-RUNTIME-BINDING-MAP.md")
assert "BMI_Graphics_UMG_C" in umg_map and "BMI_Menu_Seting_UMG_C" in umg_map
assert "event graph has not yet" in umg_map

low_latency_doc = read(RENDERER / "LOW-LATENCY-BEHAVIOR.md")
low_latency = next(item for item in minimal["settings"] if item["id"] == "low_latency")
assert low_latency["option_availability"]["Ultra"] == "disabled_until_verified"
assert "r.GTSyncType=0" in low_latency["status"]["Off"] and "r.OneFrameThreadLag=1" in low_latency["status"]["Off"]
assert "r.GTSyncType=1" in low_latency["status"]["On"] and "r.OneFrameThreadLag=1" in low_latency["status"]["On"]
low_latency_runtime = runtime[runtime.index("static void SetLowLatency"):runtime.index("static bool IsFrameGenerationAvailable")]
for token in ["r.GTSyncType", "r.OneFrameThreadLag", "SetCVarIntAndVerify", "CVar->GetInt() == Value"]:
    assert token in runtime, f"Low-latency runtime verification missing: {token}"
assert "Ultra (Zero Queue) is unavailable" in low_latency_runtime
assert 'SetCVarIntIfPresent(TEXT("r.OneFrameThreadLag"), 0)' not in low_latency_runtime
assert "Swappy" in low_latency_doc and "non-pipelined" in low_latency_doc
assert "Ultra (Zero Queue)" in low_latency_doc




unified_h = read(RENDERER / "BMIUnifiedSpatialUpscaler.h")
unified_cpp = read(RENDERER / "BMIUnifiedSpatialUpscaler.cpp")
for token in [
    "class FBMIUnifiedSpatialUpscaler final : public ISpatialUpscaler",
    "RegisterUpscaler", "RegisterCAS", "RegisterHud",
    "RegisterBMIUnifiedSpatialUpscaler",
]:
    assert token in unified_h, f"Unified renderer header missing: {token}"
for token in [
    "ISpatialUpscaler::AddDefaultUpscalePass", "EUpscaleMethod::Nearest",
    "EUpscaleMethod::Bilinear", "bmi.Pipeline.RegisteredUpscalerMask",
    "bmi.Pipeline.CASAvailable", "bmi.Pipeline.HUDAvailable",
    "safe Bilinear fallback", "FindUpscaler(Method)",
]:
    assert token in unified_cpp, f"Unified renderer implementation missing: {token}"
assert "Method<0||Method>3" in unified_cpp
assert "if (Method==4||Method==5)" in unified_cpp
assert "\\\\n TAutoConsoleVariable<int32> CVarBMIMethod" not in unified_cpp, "Malformed escaped newline in C++ declarations"
assert "CVarBMIPipelineEnabled.GetValueOnGameThread() == 0" in unified_cpp
assert "InViewFamily.GetPrimarySpatialUpscalerInterface() == nullptr" in unified_cpp
assert "InViewFamily.SetPrimarySpatialUpscalerInterface(new FBMIUnifiedSpatialUpscaler());" in unified_cpp
assert "Copy->bCAS=CVarBMICASEnabled.GetValueOnGameThread()!=0" in unified_cpp

# Algorithm source provenance and required dependencies. Presence is not a claim
# that the renderer adapter has compiled or been enabled on the target engine.
fsr = VENDOR / "FSR-Mobile"
for rel in ["opt_fsr.fxh", "ffx_a.h", "ffx_fsr1.h", "AMD-LICENSE.txt", "README.md"]:
    assert (fsr / rel).exists(), f"Missing FSR Mobile dependency: {rel}"
fsr_shader = read(fsr / "opt_fsr.fxh")
assert "void FsrMobile(" in fsr_shader
assert "FsrEasuSampleH" in fsr_shader
assert "Fused RCAS limiter and resolve" in fsr_shader
assert "atyuwen" in read(fsr / "README.md")

mwsr = VENDOR / "MWSR"
assert (mwsr / "mwsr-up.hlsl").exists() and (mwsr / "LICENSE").exists()
mwsr_shader = read(mwsr / "mwsr-up.hlsl")
assert "ddx_coarse" in mwsr_shader and "ddy_coarse" in mwsr_shader

sgsr = VENDOR / "SGSR1"
for rel in [
    "SGSR.uplugin", "Shaders/Private/PostProcessSGSR.usf",
    "Shaders/Private/sgsr.ush",
    "Source/SGSRSpatialUpscaling/SGSRSpatialUpscaler.cpp",
    "Source/SGSRSpatialUpscaling/Subpasses/SGSRSubpassScaler.cpp",
]:
    assert (sgsr / rel).exists(), f"Missing SGSR1 source: {rel}"
assert (VENDOR / "AMD-CAS/ffx_a.h").exists()
assert (VENDOR / "AMD-CAS/ffx_cas.h").exists()

# The active menu is fixed to the vendored Qualcomm SGSR1 plugin.
upscaler_setting = next(item for item in minimal["settings"] if item["id"] == "upscaler")
assert upscaler_setting["type"] == "fixed" and upscaler_setting["value"] == "SGSR1"
assert "Method != 0" in runtime
assert "FDefaultValueHelper::ParseInt(Args[0], Method)" in runtime
sgsr_scaler = read(sgsr / "Source/SGSRSpatialUpscaling/Subpasses/SGSRSubpassScaler.cpp")
sgsr_shared = read(sgsr / "Source/SGSRSpatialUpscaling/Subpasses/SGSRSubpassSharedTypes.h")
sgsr_usf = read(sgsr / "Shaders/Private/PostProcessSGSR.usf")
sgsr_ush = read(sgsr / "Shaders/Private/sgsr.ush")
assert 'TEXT("r.Qualcomm.SGSR.Sharpness")' in sgsr_scaler
assert "PassParameters->SGSR.Sharpness" in sgsr_scaler
assert "SHADER_PARAMETER(float, Sharpness)" in sgsr_shared
assert "float Sharpness;" in sgsr_usf
assert "SgsrYuvH(OutColor,uv,Const1,Sharpness)" in sgsr_usf
assert "SGSR_F1 sharpness" in sgsr_ush and "SGSR_H1 edgeSharpness = sharpness" in sgsr_ush

patch = read(RENDERER / "patches/0001-register-bmi-spatial-upscaler.patch")
assert "BMIUnifiedSpatialUpscaler.h" in patch
assert "RegisterBMIUnifiedSpatialUpscaler()" in patch
assert '+#include "BMIUnifiedSpatialUpscaler.h"' in patch

audit = json.loads(read(ENGINE / "renderer-api-audit.json"))
status = audit["implementation_status"]
assert status["target_engine_compile"] is False
assert status["android_vulkan_validation"] is False
assert status["modified_apk"] is False
assert status["mobile_methods_exposed"] == ["Nearest", "Bilinear"]

# Primary-slot arbitration: the official Qualcomm SGSR extension owns the
# primary spatial slot when selected. BMI's optional dispatcher remains off
# for this profile, preventing competing primary-upscaler assignments.
sgsr_extension = read(sgsr / "Source/SGSRSpatialUpscaling/SGSRViewExtension.cpp")
assert "if (InViewFamily.GetPrimarySpatialUpscalerInterface() == nullptr)" in sgsr_extension
assert "InViewFamily.SetPrimarySpatialUpscalerInterface(new FSGSRSpatialUpscaler" in sgsr_extension
assert 'FindCVar(TEXT("r.Qualcomm.SGSR.Enabled"))' in runtime
assert 'SGSR1->Set(1, ECVF_SetByGameSetting)' in runtime
assert 'SetCVarIntIfPresent(TEXT("bmi.Pipeline.Enabled"), 0)' in runtime
assert 'SetCVarFloatAndVerify(TEXT("r.Qualcomm.SGSR.Sharpness"), ShaderSharpness)' in runtime
assert 'SetCVarFloatAndVerify(TEXT("r.Qualcomm.SGSR.Sharpness"), EdgeSharpness[Preset])' in runtime
assert "EdgeSharpness[Preset]" in runtime
assert "RegisterBMIUnifiedSpatialUpscaler();" in patch
assert "Renderer startup patch registers the optional BMI dispatcher" in read(RENDERER / "README.md")
assert "claims the primary slot first" in read(RENDERER / "README.md")
assert audit["verified_contract"]["bmi_extension_behavior"].startswith("BMI's view extension is registered from the Renderer module startup hook")
assert audit["implementation_status"]["renderer_extension_registration_applied_to_target"] is False

sharpness_patch = read(RENDERER / "patches/0002-sgsr1-runtime-sharpness.patch")
assert "SHADER_PARAMETER(float, EdgeSharpness)" in sharpness_patch
assert "SGSR_EdgeSharpnessCVar" in sharpness_patch
assert "PassParameters->SGSR.EdgeSharpness" in sharpness_patch
assert "#define SGSR_EDGE_SHARPNESS EdgeSharpness" in sharpness_patch
assert "r.Qualcomm.SGSR.Sharpness" in sharpness_patch


# Official Qualcomm SGSR2 UE4.27 two-pass fragment implementation is vendored.
sgsr2 = VENDOR / "SGSR2-UE427/2pass_fs_taau"
for rel in [
    "GSR/GSR.uplugin",
    "GSR/Shaders/Private/gsr_common.ush",
    "GSR/Shaders/Private/gsr_convert.usf",
    "GSR/Shaders/Private/gsr_upscale.usf",
    "GSR/Source/GSRTUModule/Private/GSRTU.cpp",
    "GSR/Source/GSRTUModule/Private/GSRViewExtension.cpp",
    "LICENSE",
]:
    assert (sgsr2 / rel).exists(), f"Missing vendored SGSR2 file: {rel}"
assert (VENDOR / "SGSR2-UE427/UPSTREAM.md").exists()
assert "ITemporalUpscaler" in read(sgsr2 / "GSR/Source/GSRTUModule/Private/GSRTU.h")
assert 'TEXT("r.SGSR2.Enabled")' in read(sgsr2 / "GSR/Source/GSRTUModule/Private/GSRViewExtension.cpp")
assert "temporal, not spatial" in read(VENDOR / "SGSR2-UE427/UPSTREAM.md")


# FSR Mobile has a UE4.27 RDG pixel-pass adapter wired into BMI registration.
fsr_adapter = read(RENDERER / "BMIFSRMobilePass.cpp")
fsr_usf = read(RENDERER / "BMIShaders/Private/BMIFSRMobile.usf")
fsr_mobile_helper = read(VENDOR / "FSR-Mobile/opt_fsr_bmi.fxh")
assert "RegisterBMIFSRMobileUpscaler" in fsr_adapter
assert "RegisterBMIFSRMobileUpscaler();" in unified_cpp
assert "RegisterUpscaler(3, AddBMIFSRMobilePass)" in fsr_adapter
assert "FsrEasuConOffset" in fsr_usf and "FsrMobile(reconstructed" in fsr_usf
assert "GatherRed" in fsr_usf and "GatherGreen" in fsr_usf and "GatherBlue" in fsr_usf
assert "AF1 sharpness" in fsr_mobile_helper
assert "con0.x * sharpness" in fsr_mobile_helper


# MWSR now has an RDG pass using the upstream 96-byte constant contract.
mwsr_adapter = read(RENDERER / "BMIMWSRPass.cpp")
mwsr_usf = read(RENDERER / "BMIShaders/Private/BMIMWSR.usf")
assert "RegisterBMIMWSRUpscaler" in mwsr_adapter
assert "RegisterBMIMWSRUpscaler();" in unified_cpp
assert "RegisterUpscaler(2, AddBMIMWSRPass)" in mwsr_adapter
for field in ["texel", "scale", "dims", "across", "kc", "ft", "kg", "kwc", "kl"]:
    assert field in mwsr_adapter
assert "ddx_coarse" in mwsr_usf and "ddy_coarse" in mwsr_usf
assert "#define ddx_coarse ddx" in mwsr_usf

print("PASS: official SGSR1 route, runtime shader sharpness mapping, MWSR/FSR source checks, unified dispatch, source provenance, safe fallback, and feature gates")


# Target-RHI and adapter safety documentation must stay explicit.
renderer_readme = read(RENDERER / "README.md")
fsr_readme = read(VENDOR / "FSR-Mobile/README.md")
audit = json.loads(read(ENGINE / "renderer-api-audit.json"))
assert "Android OpenGL ES" in renderer_readme
assert "UE4.27 RDG pixel-pass adapter" in fsr_readme
assert "nor Android OpenGL ES shader compilation has been performed" in fsr_readme
assert audit["target_rhi"]["api"] == "OpenGL ES"
assert audit["target_rhi"]["vulkan_enabled"] is False
assert audit["implementation_status"]["android_opengles_validation"] is False
assert audit["implementation_status"]["fsr_mobile_adapter_registered"] is False
assert audit["implementation_status"]["fsr_mobile_shader_compiled"] is False

assert "SetPrimarySpatialUpscalerInterface is assign-once" in unified_cpp
