#!/usr/bin/env python3
"""Static contract checks for the Bright Memory Android graphics integration."""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
cfg = json.loads((ROOT / "config/native-graphics-integration.json").read_text())

expected = [
    "display_resolution", "graphics_quality", "render_scale",
    "one_spatial_upscaler", "upscale_sharpness",
    "upscale_preset_mapping", "amd_cas_sharpen_only"
]
assert cfg["pipeline_order"] == expected
assert cfg["upscaling"]["single_active_method"] is True
assert cfg["upscaling"]["fallback"] == "Unreal default upscale"
assert cfg["display_resolution"]["native_behavior"].startswith("Native keeps display mode")
assert cfg["display_resolution"]["mapping_status"].endswith("not-proven")
assert cfg["render_scale"]["independent_of_display_resolution"] is True
assert cfg["render_scale"]["range"][0] < cfg["render_scale"]["range"][1]
assert cfg["graphics_quality"]["options"] == [
    "Low", "Medium", "High", "Very High", "Ultra", "Extreme"
]
assert set(cfg["graphics_quality"]["mapping_policy"]) == set(cfg["graphics_quality"]["options"])
assert cfg["amd_cas"]["mode"] == "sharpen-only"
assert cfg["amd_cas"]["stage"].startswith("after-selected-spatial-upscaler")
assert cfg["amd_cas"]["gles31_shader"].endswith("CAS_Shader_GLES31.comp")
assert cfg["current_delivery_status"]["modified_APK"] is False
for key in ("display_resolution", "graphics_quality", "render_scale",
            "upscaling", "upscale_sharpness", "upscale_preset_mapping", "amd_cas"):
    assert cfg[key]["expose"] is False if "expose" in cfg[key] else True

print("PASS: ordered graphics contract, single-upscaler invariant, render-scale separation,")
print("      six quality tiers, CAS post-upscale stage, and no premature UI exposure.")
