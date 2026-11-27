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
for token in [
    "SetResolutionController", "r.MobileContentScaleFactor",
    "SetOverallScalabilityLevel", "SetUpscalePreset", "SetUpscaler",
    "bmi.Pipeline.RegisteredUpscalerMask", "bmi.Pipeline.Enabled", "SetUpscaleSharpness",
    "bmi.Pipeline.CASAvailable", "SetCAS", "SetCASSharpness",
    "SetFramePacing", "r.OneFrameThreadLag", "SetPerformanceHUD",
    "bmi.Pipeline.HUDAvailable", "SetFrameRateLimit",
]:
    assert token in runtime, f"Missing runtime bridge symbol: {token}"
assert "Requested spatial upscaler has no registered renderer pass" in runtime
assert "BMI unified renderer pipeline is not installed" in runtime
assert "PipelineEnabled->Set(1, ECVF_SetByGameSetting)" in runtime
assert "Ultra (No Queue) is unavailable" in runtime
assert 'FindCVar(TEXT("r.SGSR2.Enabled"))' in runtime
assert 'FindCVar(TEXT("r.Qualcomm.SGSR.Enabled"))' in runtime
assert "TemporalUpsampling->Set(1, ECVF_SetByGameSetting)" in runtime
assert 'SetCVarIntIfPresent(TEXT("r.UsePreExposure"), 1)' in runtime
assert 'SetCVarIntIfPresent(TEXT("r.SGSR2.Exposure"), 1)' in runtime
assert "SGSR2 is temporal and must" in runtime
assert 'SetCVarIntIfPresent(TEXT("r.SGSR2.Quality"), GSR2Quality[Preset])' in runtime
assert "active Android OpenGL ES pacer/Swappy path has not yet been integrated" in runtime

blueprint_header = read(ENGINE / "Source/BMIGraphicsRuntime/Public/BMIGraphicsSettingsLibrary.h")
blueprint_source = read(ENGINE / "Source/BMIGraphicsRuntime/Private/BMIGraphicsSettingsLibrary.cpp")
for method in [
    "SetResolutionController", "SetGraphicsQuality", "SetRenderScale",
    "SetUpscaler", "SetUpscaleSharpness", "SetUpscalePreset", "SetAMDCAS",
    "SetCASSharpness", "SetFramePacing", "SetLowLatency",
    "SetPerformanceHUD", "SetFrameLimit",
]:
    assert f"static bool {method}(" in blueprint_header, f"Missing Blueprint API: {method}"
    assert f"UBMIGraphicsSettingsLibrary::{method}(" in blueprint_source, f"Missing implementation: {method}"

config = json.loads(read(ROOT / "config/graphics-settings.json"))
expected = [
    "resolution_controller", "graphics_quality", "render_scale", "upscaler",
    "upscale_sharpness", "upscale_preset", "amd_cas", "cas_sharpness",
    "frame_generation", "frame_pacing", "low_latency", "performance_hud",
]
assert config["menu_order"] == expected, "Menu order changed"
items = config["settings"]
ids = [item["id"] for item in items]
assert ids[:len(expected)] == expected
upscaler = next(item for item in items if item["id"] == "upscaler")
assert upscaler["options"] == ["SGSR1", "SGSR2", "MWSR", "FSR Mobile", "Nearest", "Bilinear"]
assert "FSR Mobile" in upscaler["methods"] and "FSR1" not in upscaler["options"]
for item in items:
    if item["id"] in {"upscaler", "amd_cas", "frame_generation", "frame_pacing",
                      "low_latency", "performance_hud"}:
        assert item.get("expose_control", False) is False, (
            f"Unverified feature must remain hidden: {item['id']}"
        )

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

patch = read(RENDERER / "patches/0001-register-bmi-spatial-upscaler.patch")
assert "BMIUnifiedSpatialUpscaler.h" in patch
assert "RegisterBMIUnifiedSpatialUpscaler()" in patch
assert '+#include "BMIUnifiedSpatialUpscaler.h"' in patch
assert "+ RegisterBMIUnifiedSpatialUpscaler();" in patch

audit = json.loads(read(ENGINE / "renderer-api-audit.json"))
status = audit["implementation_status"]
assert status["target_engine_compile"] is False
assert status["android_vulkan_validation"] is False
assert status["modified_apk"] is False
assert status["mobile_methods_exposed"] == ["Nearest", "Bilinear"]

# Primary-slot arbitration: the vendored SGSR extension and BMI must never
# attempt a second assignment. BMI registration comes from the Renderer
# startup patch; the runtime disables SGSR before enabling BMI.
sgsr_extension = read(sgsr / "Source/SGSRSpatialUpscaling/SGSRViewExtension.cpp")
assert "if (InViewFamily.GetPrimarySpatialUpscalerInterface() == nullptr)" in sgsr_extension
assert "InViewFamily.SetPrimarySpatialUpscalerInterface(new FSGSRSpatialUpscaler" in sgsr_extension
assert 'SetCVarIntIfPresent(TEXT("r.Qualcomm.SGSR.Enabled"), 0)' in runtime
assert "RegisterBMIUnifiedSpatialUpscaler();" in patch
assert "Renderer startup patch replaces the old BMI registration call" in read(RENDERER / "README.md")
assert "claims the primary slot first" in read(RENDERER / "README.md")
assert audit["verified_contract"]["bmi_extension_behavior"].startswith("BMI's view extension is registered from the Renderer module startup hook")
assert audit["implementation_status"]["renderer_extension_registration_applied_to_target"] is False


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

print("PASS: SGSR1/SGSR2 routes, MWSR and FSR Mobile RDG adapters, unified dispatch, source provenance, safe fallback, and feature gates")


# Target-RHI and adapter safety documentation must stay explicit.
renderer_readme = read(RENDERER / "README.md")
fsr_readme = read(VENDOR / "FSR-Mobile/README.md")
audit = json.loads(read(ENGINE / "renderer-api-audit.json"))
assert "Android OpenGL ES" in renderer_readme
assert "UE4.27 RDG pixel-pass adapter" in fsr_readme
assert "Android OpenGL ES shader compilation has been performed" not in fsr_readme
assert "shader compilation has not been performed" in fsr_readme
assert audit["target_rhi"]["api"] == "OpenGL ES"
assert audit["target_rhi"]["vulkan_enabled"] is False
assert audit["implementation_status"]["android_opengles_validation"] is False
assert audit["implementation_status"]["fsr_mobile_adapter_registered"] is False
assert audit["implementation_status"]["fsr_mobile_shader_compiled"] is False

assert "SetPrimarySpatialUpscalerInterface is assign-once" in unified_cpp
