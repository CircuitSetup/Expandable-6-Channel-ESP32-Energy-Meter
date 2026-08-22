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


def compile_matrix() -> dict[str, list[str]]:
    paths = [ESPHOME_DIR / filename for filename in REPRESENTATIVES]
    if len(paths) != len(set(paths)) or not all(path.is_file() for path in paths):
        raise ValueError("invalid compile matrix representatives")
    return {
        "configurations": sorted(
            path.relative_to(REPOSITORY_DIR).as_posix() for path in paths
        )
    }


if __name__ == "__main__":
    print(json.dumps(compile_matrix(), sort_keys=True, separators=(",", ":")))
