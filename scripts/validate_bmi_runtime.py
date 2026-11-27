import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
descriptor = json.loads((root / "engine-extension/BMIGraphicsRuntime.uplugin").read_text())
assert descriptor["FileVersion"] == 3
assert any(m["Name"] == "BMIGraphicsRuntime" and m["Type"] == "Runtime" for m in descriptor["Modules"])

source = (root / "engine-extension/Source/BMIGraphicsRuntime/Private/BMIGraphicsRuntimeModule.cpp").read_text()
required = [
    "SetResolutionScaleValueEx",
    "SetOverallScalabilityLevel",
    "SetFrameRateLimit",
    "r.ShadowQuality",
    "r.Mobile.AmbientOcclusionQuality",
]
for symbol in required:
    assert symbol in source, f"Missing runtime integration: {symbol}"

config = json.loads((root / "config/graphics-settings.json").read_text())
ids = [item["id"] for item in config["settings"]]
for expected in ["render_scale", "graphics_quality", "upscaler", "amd_cas",
                 "frame_generation", "frame_pacing", "low_latency", "performance_hud"]:
    assert expected in ids, f"Missing agreed setting: {expected}"

for item in config["settings"]:
    if item["id"] in {"upscaler", "amd_cas", "frame_generation", "low_latency", "performance_hud"}:
        assert item.get("expose_control", False) is False, (
            f"Unimplemented feature must remain hidden: {item['id']}"
        )

print("PASS: plugin descriptor, native API wiring, settings schema and feature gates")
