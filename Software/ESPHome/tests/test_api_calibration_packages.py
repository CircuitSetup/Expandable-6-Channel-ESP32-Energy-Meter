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


def sensor_packages(config: Path) -> list[Path]:
    return [
        ESPHOME_DIR.parents[1] / package
        for package in re.findall(
            r"^\s*- (Software/ESPHome/meter_sensors/6chan_(?:main_sensor|addon\d+)\.yaml)$",
            config.read_text(encoding="utf-8"),
            re.MULTILINE,
        )
    ]


def atm90e32_voltage_phases(package: Path) -> list[set[str]]:
    groups = package.read_text(encoding="utf-8").split("  - platform: atm90e32\n")[1:]
    voltage_phases = []
    for group in groups:
        voltage_phases.append(
            {
                phase
                for phase in "abc"
                if re.search(
                    rf"^    phase_{phase}:\n(?:(?!^    phase_[abc]:|^    (?:frequency|chip_temperature|line_frequency|gain_pga|update_interval|enable_offset_calibration):).)*?^      voltage:\n",
                    group,
                    re.MULTILINE | re.DOTALL,
                )
            }
        )
    return voltage_phases


def test_official_meter_configs_include_every_declared_calibration_package():
    """Fails if a configured ATM90E32 group loses its API calibration package."""
    for config in ESPHOME_DIR.glob("6chan_energy_meter*.yaml"):
        addon_count = declared_addon_count(config)
        expected = {"6chan_main_calibration.yaml"} | {
            f"6chan_addon{index}_calibration.yaml" for index in range(1, addon_count + 1)
        }
        assert calibration_packages(config) == expected, config.name


def test_official_meter_configs_expose_voltage_for_every_atm90e32_phase():
    """Fails if a meter phase loses the API voltage diagnostics calibration uses."""
    for config in ESPHOME_DIR.glob("6chan_energy_meter*.yaml"):
        phases = [
            phase_set
            for package in sensor_packages(config)
            for phase_set in atm90e32_voltage_phases(package)
        ]
        assert len(phases) == 2 * (declared_addon_count(config) + 1), config.name
        assert phases == [{"a", "b", "c"}] * len(phases), config.name
        assert sum(len(phase_set) for phase_set in phases) == 6 * (
            declared_addon_count(config) + 1
        ), config.name
