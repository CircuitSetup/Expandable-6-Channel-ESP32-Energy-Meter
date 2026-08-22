import json
from pathlib import Path


CATALOG = Path(__file__).resolve().parents[1] / "ct_presets.json"

EXPECTED_PRESETS = {
    "sct_006_20a_25ma": ("SCT-006 20A/25mA", 11143, False),
    "sct_013_030_30a_1v": ("SCT-013-030 30A/1V", 8650, True),
    "sct_013_050_50a_1v": ("SCT-013-050 50A/1V", 15420, True),
    "sct_010_50a_16_6ma": ("SCT-010 50A/16.6mA", 41334, False),
    "sct_010_80a_26_6ma": ("SCT-010 80A/26.6mA", 41660, False),
    "sct_013_000_100a_50ma": ("SCT-013-000 100A/50mA", 27518, False),
    "sct_016_120a_40ma": ("SCT-016 120A/40mA", 41787, False),
    "sct_024_200a_100ma": ("SCT-024 200A/100mA", 27518, False),
    "sct_024_200a_50ma": ("SCT-024 200A/50mA", 55036, False),
}


def test_ct_preset_catalog_is_complete_and_safe_to_consume():
    assert CATALOG.exists(), "missing versioned CT preset catalog"
    catalog = json.loads(CATALOG.read_text(encoding="utf-8"))
    assert catalog.keys() == {"schema_version", "presets"}
    assert catalog["schema_version"] == 1

    presets = catalog["presets"]
    assert len(presets) == len(EXPECTED_PRESETS)
    assert {preset["model_id"] for preset in presets} == set(EXPECTED_PRESETS)

    for preset in presets:
        label, gain, cut_jumper = EXPECTED_PRESETS[preset["model_id"]]
        assert preset.keys() == {
            "model_id",
            "label",
            "rated_current_a",
            "secondary",
            "default_gain_ct",
            "requires_burden_jumper_cut",
            "notes",
        }
        assert preset["label"] == label
        assert isinstance(preset["rated_current_a"], (int, float))
        assert preset["rated_current_a"] > 0
        assert isinstance(preset["secondary"], str) and preset["secondary"]
        assert preset["default_gain_ct"] == gain
        assert 0 < preset["default_gain_ct"] <= 65535
        assert preset["requires_burden_jumper_cut"] is cut_jumper
        assert isinstance(preset["notes"], str) and preset["notes"]
        if cut_jumper:
            assert "cut the matching burden jumper" in preset["notes"].lower()
        else:
            assert "retain onboard burden" in preset["notes"].lower()

    ambiguous = {
        preset["model_id"] for preset in presets if preset["default_gain_ct"] == 27518
    }
    assert ambiguous == {"sct_013_000_100a_50ma", "sct_024_200a_100ma"}
