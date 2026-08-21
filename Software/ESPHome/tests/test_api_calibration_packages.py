import re
from pathlib import Path


ESPHOME_DIR = Path(__file__).resolve().parents[1]


def declared_addon_count(config: Path) -> int:
    project_name = re.search(
        r"^    name: (circuitsetup\.6c-energy-meter[^\r\n]*)$",
        config.read_text(encoding="utf-8"),
        re.MULTILINE,
    )
    assert project_name, f"{config.name} has no ESPHome project name"
    match = re.search(r"-(\d+)-addons?", project_name.group(1))
    return int(match.group(1)) if match else 0


def calibration_packages(config: Path) -> set[str]:
    return set(
        re.findall(
            r"^\s*- Software/ESPHome/calibration/(6chan_(?:main|addon\d+)_calibration\.yaml)$",
            config.read_text(encoding="utf-8"),
            re.MULTILINE,
        )
    )


def test_official_meter_configs_include_every_declared_calibration_package():
    """Fails if a configured ATM90E32 group loses its API calibration package."""
    for config in ESPHOME_DIR.glob("6chan_energy_meter*.yaml"):
        addon_count = declared_addon_count(config)
        expected = {"6chan_main_calibration.yaml"} | {
            f"6chan_addon{index}_calibration.yaml" for index in range(1, addon_count + 1)
        }
        assert calibration_packages(config) == expected, config.name
