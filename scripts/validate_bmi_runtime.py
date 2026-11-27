import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
descriptor = json.loads((root / "engine-extension/BMIGraphicsRuntime.uplugin").read_text())
assert descriptor["FileVersion"] == 3
assert any(m["Name"] == "BMIGraphicsRuntime" and m["Type"] == "Runtime" for m in descriptor["Modules"])

source = (root / "engine-extension/Source/BMIGraphicsRuntime/Private/BMIGraphicsRuntimeModule.cpp").read_text()
required = [
    "SetResolutionController",
    "r.MobileContentScaleFactor",
    "SetResolutionScaleValueEx",
    "SetOverallScalabilityLevel",
    "SetUpscalePreset",
    "SetUpscaler",
    "SetUpscaleSharpness",
    "r.Qualcomm.SGSR.Sharpness",
    "SetCAS",
    "SetCASSharpness",
    "SetLowLatency",
    "r.OneFrameThreadLag",
    "SetFrameRateLimit",
]
for symbol in required:
    assert symbol in source, f"Missing runtime integration: {symbol}"

contract = (root / "engine-extension/RENDERER-INTEGRATION-CONTRACT.md").read_text()
for symbol in [
    "FSceneViewExtensionBase",
    "FSceneViewExtensions::NewExtension",
    "BeginRenderViewFamily",
    "ISpatialUpscaler",
    "Fork_GameThread",
    "FRDGBuilder",
    "Renderer/Private/PostProcess/PostProcessUpscale.h",
    "private header",
]:
    assert symbol in contract, f"Missing verified renderer contract detail: {symbol}"

config = json.loads((root / "config/graphics-settings.json").read_text())
ids = [item["id"] for item in config["settings"]]
upscaler_config = next(item for item in config["settings"] if item["id"] == "upscaler")
assert "SGSR1_official_UE4_27_module" in upscaler_config["status"]
assert "AddDefaultUpscalePass" in upscaler_config["methods"]["Nearest"]
assert "AddDefaultUpscalePass" in upscaler_config["methods"]["Bilinear"]
assert any("BMISpatialUpscaler.h/.cpp" in x for x in upscaler_config["integration_requirements"])
for expected in ["render_scale", "graphics_quality", "upscaler", "amd_cas",
                 "frame_generation", "frame_pacing", "low_latency", "performance_hud"]:
    assert expected in ids, f"Missing agreed setting: {expected}"

for item in config["settings"]:
    if item["id"] in {"upscaler", "amd_cas", "frame_generation", "low_latency", "performance_hud"}:
        assert item.get("expose_control", False) is False, (
            f"Unimplemented feature must remain hidden: {item['id']}"
        )

audit = json.loads((root / "engine-extension/renderer-api-audit.json").read_text())
assert audit["schema_version"] == 1
assert audit["reference"]["repository"] == "jsuk10/UE4.27plus_Custom"
assert len(audit["reference"]["files"]) == 5
verified = audit["verified_contract"]
for symbol in [
    "FSceneViewExtensionBase",
    "FSceneViewExtensions::NewExtension<T>(...)",
    "SetupViewFamily(FSceneViewFamily&)",
    "SetupView(FSceneViewFamily&, FSceneView&)",
    "BeginRenderViewFamily(FSceneViewFamily&)",
    "PreRenderViewFamily_RenderThread(FRHICommandListImmediate&, FSceneViewFamily&)",
    "PreRenderView_RenderThread(FRHICommandListImmediate&, FSceneView&)",
]:
    assert symbol in verified["required_overrides"] or symbol in [
        verified["view_extension_base"], verified["factory"]
    ], f"Missing verified view-extension API: {symbol}"
for symbol in ["GetDebugName() const", "Fork_GameThread(const FSceneViewFamily&) const",
               "AddPasses(FRDGBuilder&, const FViewInfo&, const FInputs&) const"]:
    assert symbol in verified["upscaler_methods"], f"Missing ISpatialUpscaler method: {symbol}"
for field in ["FScreenPassRenderTarget OverrideOutput", "FScreenPassTexture SceneColor",
              "EUpscaleStage Stage"]:
    assert field in verified["input_fields"], f"Missing ISpatialUpscaler input: {field}"
assert verified["view_family_assignment"] == "SetPrimarySpatialUpscalerInterface(ISpatialUpscaler*)"
assert verified["screen_pass_helper"] == "AddDrawScreenPass"
assert verified["rdg_pass_flag"] == "ERDGPassFlags::Raster"
constraints = " ".join(audit["integration_constraints"])
assert "private header" in constraints
assert "not proven identical" in constraints
status = audit["implementation_status"]
assert status["reference_signatures_inspected"] is True
assert status["renderer_bridge_source_authored"] is True
assert status["mobile_methods_exposed"] == ["Nearest", "Bilinear"]
assert status["default_enabled"] is False
assert audit["verified_contract"]["view_family_ownership"].startswith("FSceneViewFamily::~FSceneViewFamily")
assert audit["verified_contract"]["fork_behavior"].startswith("FSceneRenderer forks")
assert status["renderer_patch_written"] is True
assert status["renderer_patch_path"] == "engine-extension/renderer/patches/0001-register-bmi-spatial-upscaler.patch"
assert status["target_engine_compile"] is False
assert status["android_vulkan_validation"] is False
assert status["modified_apk"] is False


renderer_dir = root / "engine-extension/renderer"
renderer_header = (renderer_dir / "BMISpatialUpscaler.h").read_text()
renderer_source = (renderer_dir / "BMISpatialUpscaler.cpp").read_text()
renderer_readme = (renderer_dir / "README.md").read_text()
for symbol in [
    "class FBMISpatialUpscaler final : public ISpatialUpscaler",
    "Fork_GameThread(const FSceneViewFamily& ViewFamily) const override",
    "AddPasses(",
    "class FBMISpatialUpscalerViewExtension final : public FSceneViewExtensionBase",
    "BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override",
    "RegisterBMISpatialUpscaler()",
]:
    assert symbol in renderer_header, f"Renderer bridge header missing: {symbol}"
for symbol in [
    "ISpatialUpscaler::AddDefaultUpscalePass",
    "EUpscaleMethod::Nearest",
    "EUpscaleMethod::Bilinear",
    "SetPrimarySpatialUpscalerInterface(new FBMISpatialUpscaler())",
    "GetPrimarySpatialUpscalerInterface() == nullptr",
    "bmi.UseNativeSpatialUpscaler",
    "bmi.NativeUpscaleMethod",
]:
    assert symbol in renderer_source, f"Renderer bridge source missing: {symbol}"
assert "defaults to disabled (opt-in)" in renderer_readme
assert "official Qualcomm UE4.27 SGSR1 plugin source" in renderer_readme
assert "not yet connected to an RDG shader pass" in renderer_readme
assert "have not been compiled against the exact Bright Memory v1.34" in renderer_readme
assert "FSceneViewFamily owns" in renderer_source
assert "does not replace a primary upscaler" in renderer_readme

patch = (root / "engine-extension/renderer/patches/0001-register-bmi-spatial-upscaler.patch").read_text()
for token in ["BMISpatialUpscaler.h", "RegisterBMISpatialUpscaler()", "FRendererModule::StartupModule"]:
    assert token in patch, f"Renderer registration patch missing: {token}"

sgsr_vendor = root / "engine-extension/renderer/vendor/SGSR1"
for rel in [
    "SGSR1.uplugin",
    "Shaders/Private/PostProcessSGSR.usf",
    "Shaders/Private/sgsr.ush",
    "Source/SGSRSpatialUpscaling/SGSRSpatialUpscaler.cpp",
    "Source/SGSRSpatialUpscaling/Subpasses/SGSRSubpassScaler.cpp",
]:
    assert (sgsr_vendor / rel).exists(), f"Missing vendored SGSR1 integration file: {rel}"
sgsr_view = (sgsr_vendor / "Source/SGSRSpatialUpscaling/SGSRViewExtension.cpp").read_text()
assert 'TEXT("r.Qualcomm.SGSR.Enabled"),\n\t0,' in sgsr_view
assert "GetPrimarySpatialUpscalerInterface() == nullptr" in sgsr_view
sgsr_scaler = (sgsr_vendor / "Source/SGSRSpatialUpscaling/Subpasses/SGSRSubpassScaler.cpp").read_text()
assert "r.Qualcomm.SGSR.Sharpness" in sgsr_scaler
assert "PassParameters->SGSR.Sharpness" in sgsr_scaler
cas_vendor = root / "engine-extension/renderer/vendor/AMD-CAS"
assert (cas_vendor / "ffx_a.h").exists() and (cas_vendor / "ffx_cas.h").exists()

print("PASS: runtime settings bridge, SGSR1 source package and sharpness wiring, AMD CAS source provenance, renderer API audit, low-latency safety gates and settings schema")
