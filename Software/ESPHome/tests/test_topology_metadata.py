import importlib.util
import sys
from pathlib import Path

import pytest


ESPHOME_DIR = Path(__file__).resolve().parents[1]
MATRIX_SCRIPT = ESPHOME_DIR / "tests" / "compile_matrix.py"


def matrix_module():
    assert MATRIX_SCRIPT.exists(), "missing topology compile-matrix generator"
    spec = importlib.util.spec_from_file_location("compile_matrix", MATRIX_SCRIPT)
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def test_declared_addon_count_handles_official_names_and_rejects_unknowns():
    matrix = matrix_module()
    cases = {
        "circuitsetup.6c-energy-meter": 0,
        "circuitsetup.6c-energy-meter-ethernet": 0,
        "circuitsetup.6c-energy-meter-1-addon": 1,
        "circuitsetup.6c-energy-meter-2-addons-ethernet": 2,
        "circuitsetup.6c-energy-meter-3-addons-2-voltages": 3,
        "circuitsetup.6c-energy-meter-6-addons-ethernet-waveshare": 6,
    }
    for name, expected in cases.items():
        assert matrix.declared_addon_count(name) == expected
    with pytest.raises(ValueError):
        matrix.declared_addon_count("circuitsetup.6c-energy-meter-unknown")


def test_official_topology_metadata_is_contiguous_and_complete():
    matrix = matrix_module()
    configurations = matrix.official_configurations()
    assert configurations
    for config in configurations:
        addon_count = matrix.declared_addon_count(matrix.project_name(config))
        expected_indices = set(range(1, addon_count + 1))
        assert matrix.package_addon_indices(config, "meter_sensors") == expected_indices
        assert matrix.package_addon_indices(config, "calibration") == expected_indices
        expected_channels = set(range(1, 6 * (addon_count + 1) + 1))
        assert matrix.substitution_indices(config, "ct", "_name") == expected_channels
        assert matrix.substitution_indices(config, "current_cal_ct", "") == expected_channels


def test_compile_matrix_is_deterministic_and_covers_every_required_variant():
    matrix = matrix_module()
    data = matrix.compile_matrix()
    assert set(data) == {"configurations"}
    configurations = data["configurations"]
    assert configurations == sorted(configurations)
    assert len(configurations) == len(set(configurations))
    assert all((ESPHOME_DIR.parent.parent / path).is_file() for path in configurations)
    selected = [ESPHOME_DIR.parent.parent / path for path in configurations]
    assert {
        matrix.declared_addon_count(matrix.project_name(config)) for config in selected
    } == set(range(7))
    assert any(config.name.endswith("_ethernet.yaml") for config in selected)
    assert any(config.name.endswith("_ethernet_waveshare.yaml") for config in selected)
    assert any(config.name.endswith("_2-voltages.yaml") for config in selected)
