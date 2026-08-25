"""Emit a deterministic ESPHome compile matrix for official meter variants."""

import json
import re
from pathlib import Path


ESPHOME_DIR = Path(__file__).resolve().parents[1]
REPOSITORY_DIR = ESPHOME_DIR.parents[1]
BASE_PROJECT_NAMES = {
    "circuitsetup.6c-energy-meter",
    "circuitsetup.6c-energy-meter-ethernet",
    "circuitsetup.6c-energy-meter-ethernet-waveshare",
}
REPRESENTATIVES = (
    "6chan_energy_meter_main_board.yaml",
    "6chan_energy_meter_main_ethernet.yaml",
    "6chan_energy_meter_main_ethernet_waveshare.yaml",
    "6chan_energy_meter_1-addon.yaml",
    "6chan_energy_meter_2-addons.yaml",
    "6chan_energy_meter_3-addons.yaml",
    "6chan_energy_meter_4-addons.yaml",
    "6chan_energy_meter_5-addons.yaml",
    "6chan_energy_meter_6-addons.yaml",
    "6chan_energy_meter_6-addons_ethernet.yaml",
    "6chan_energy_meter_6-addons_ethernet_waveshare.yaml",
    "6chan_energy_meter_3-addons_2-voltages.yaml",
)
LOCAL_STATUS_HARNESS = ESPHOME_DIR / "local_status_harness.generated.yaml"
LOCAL_PACKAGES = ("6chan_common.yaml", "meter_sensors/6chan_main_sensor.yaml") + tuple(
    f"meter_sensors/6chan_addon{index}.yaml" for index in range(1, 7)
) + ("status_fields/6chan_main_status.yaml",) + tuple(
    f"status_fields/6chan_addon{index}_status.yaml" for index in range(1, 7)
)


def official_configurations() -> list[Path]:
    return sorted(ESPHOME_DIR.glob("6chan_energy_meter*.yaml"))


def project_name(config: Path) -> str:
    match = re.search(
        r"^    name: (circuitsetup\.6c-energy-meter[^\r\n]*)$",
        config.read_text(encoding="utf-8"),
        re.MULTILINE,
    )
    if not match:
        raise ValueError(f"missing project name: {config}")
    return match.group(1)


def declared_addon_count(name: str) -> int:
    if name in BASE_PROJECT_NAMES:
        return 0
    match = re.fullmatch(
        r"circuitsetup\.6c-energy-meter-([1-6])-addons?(?:-(?:ethernet(?:-waveshare)?|2-voltages))?",
        name,
    )
    if not match:
        raise ValueError(f"unknown meter project name: {name}")
    return int(match.group(1))


def package_addon_indices(config: Path, package: str) -> set[int]:
    suffix = "_calibration" if package == "calibration" else ""
    return {
        int(index)
        for index in re.findall(
            rf"^\s*- Software/ESPHome/{package}/6chan_addon(\d+){suffix}\.yaml$",
            config.read_text(encoding="utf-8"),
            re.MULTILINE,
        )
    }


def substitution_indices(config: Path, prefix: str, suffix: str) -> set[int]:
    return {
        int(index)
        for index in re.findall(
            rf"^  {re.escape(prefix)}(\d+){re.escape(suffix)}:",
            config.read_text(encoding="utf-8"),
            re.MULTILINE,
        )
    }


def generate_local_status_harness() -> Path:
    source = (ESPHOME_DIR / "6chan_energy_meter_6-addons.yaml").read_text(encoding="utf-8")
    local = "packages:\n" + "\n".join(
        f"  local_{index}: !include {package}" for index, package in enumerate(LOCAL_PACKAGES)
    ) + "\n\n"
    rendered, replacements = re.subn(
        r"^packages:\n.*?(?=^[A-Za-z_][A-Za-z0-9_]*:|\Z)", local, source, count=1, flags=re.MULTILINE | re.DOTALL
    )
    if replacements != 1:
        raise ValueError("could not replace remote package block")
    with LOCAL_STATUS_HARNESS.open("w", encoding="utf-8", newline="\n") as output:
        output.write(rendered)
    return LOCAL_STATUS_HARNESS


def compile_matrix() -> dict[str, list[str]]:
    paths = [ESPHOME_DIR / filename for filename in REPRESENTATIVES] + [generate_local_status_harness()]
    if len(paths) != len(set(paths)) or not all(path.is_file() for path in paths):
        raise ValueError("invalid compile matrix representatives")
    return {
        "configurations": sorted(
            path.relative_to(REPOSITORY_DIR).as_posix() for path in paths
        )
    }


def self_test() -> None:
    generated = generate_local_status_harness()
    text = generated.read_text(encoding="utf-8")
    assert "remote_package:" not in text
    assert text.count("!include") == len(LOCAL_PACKAGES)
    assert all(package in text for package in LOCAL_PACKAGES)
    matrix = compile_matrix()["configurations"]
    assert len(matrix) == 13
    assert not LOCAL_STATUS_HARNESS.match("6chan_energy_meter*.yaml")
    assert LOCAL_STATUS_HARNESS.relative_to(REPOSITORY_DIR).as_posix() in matrix


if __name__ == "__main__":
    if "--self-test" in __import__("sys").argv:
        self_test()
    else:
        print(json.dumps(compile_matrix(), sort_keys=True, separators=(",", ":")))
