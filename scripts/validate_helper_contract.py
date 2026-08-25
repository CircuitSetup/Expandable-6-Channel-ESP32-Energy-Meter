"""Dependency-free contract checks for official ESPHome meter configurations."""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
ESP = ROOT / "Software" / "ESPHome"
REQUIRED = ("friendly_name", "update_time", "electric_freq", "voltage_cal1", "voltage_cal2")
STATUS = ESP / "status_fields"
EXPECTED = {f"6chan_energy_meter_{n}-{'addon' if n == 1 else 'addons'}{suffix}.yaml" for n in range(1, 7) for suffix in ("", "_ethernet", "_ethernet_waveshare")} | {"6chan_energy_meter_main_board.yaml", "6chan_energy_meter_main_ethernet.yaml", "6chan_energy_meter_main_ethernet_waveshare.yaml", "6chan_energy_meter_3-addons_2-voltages.yaml"}
EXPECTED_STATUS = {f"6chan_{kind}_status.yaml" for kind in ("main", "addon1", "addon2", "addon3", "addon4", "addon5", "addon6")}


def lines(path):
    return path.read_text(encoding="utf-8").splitlines()


def count_key(text, key):
    return sum(bool(re.match(rf"^[ \t]*{re.escape(key)}[ \t]*:", line)) for line in text.splitlines())


def validate_config(path):
    text = "\n".join(lines(path))
    substitutions = text.split("\nesphome:", 1)[0]
    errors = []
    if count_key(text, "csemh_config_contract") != 1 or not re.search(r'^[ \t]*csemh_config_contract[ \t]*:[ \t]*["\']2["\'][ \t]*$', text, re.M):
        errors.append('csemh_config_contract must be "2" exactly once')
    for key in REQUIRED:
        if count_key(substitutions, key) != 1:
            errors.append(f"{key} must exist exactly once")
    active = sorted({int(n) for n in re.findall(r"^[ \t]*ct(\d+)_name[ \t]*:", text, re.M)})
    for n in active:
        if count_key(text, f"current_cal_ct{n}") != 1:
            errors.append(f"active CT{n} lacks current_cal_ct{n}")
    if count_key(text, "board_revision"):
        errors.append("board_revision is not permitted")
    includes = re.findall(r"^[ \t]*- (Software/ESPHome/(?:power_quality|status_fields)/[^\s#]+\.yaml)[ \t]*$", text, re.M)
    for include in set(includes):
        if includes.count(include) > 1:
            errors.append(f"duplicate package include: {include}")
    required_totals = ("totalAmps", "totalWatts", "totalEnergyDaily") if "-addons" in path.name else ("totalEnergyDaily",)
    for key in required_totals:
        present = len(re.findall(rf"^[ \t]+id:[ \t]*{key}[ \t]*$", text, re.M)) == 1
        if not present:
            errors.append(f"{key} must have one definition")
    return errors


def validate_status():
    errors = []
    for path in sorted(STATUS.glob("6chan_*_status.yaml")):
        text = "\n".join(lines(path))
        names = list(re.finditer(r"^[ \t]+name:", text, re.M))
        if not names or any(not re.search(r"entity_category:[ \t]+diagnostic", text[m.end():m.end()+120]) or not re.search(r"disabled_by_default:[ \t]+true", text[m.end():m.end()+120]) for m in names):
            errors.append(f"{path.name}: every status entity must be diagnostic and disabled_by_default")
    return errors


def validate(paths):
    errors = validate_status()
    shared = "\n".join(lines(ESP / "meter_sensors" / "6chan_main_sensor.yaml"))
    for key in ("totalAmpsMain", "totalWattsMain"):
        if len(re.findall(rf"^[ \t]+id:[ \t]*{key}[ \t]*$", shared, re.M)) != 1:
            errors.append(f"shared main sensor must define {key} exactly once")
    for path in paths:
        errors.extend(f"{path.name}: {error}" for error in validate_config(path))
    if errors:
        raise AssertionError("\n".join(errors))


def self_test():
    good = """substitutions:\n  csemh_config_contract: \"2\"\n  friendly_name: Meter\n  update_time: 10s\n  electric_freq: 60Hz\n  voltage_cal1: 1\n  voltage_cal2: 1\n  ct1_name: CT1\n  current_cal_ct1: 1\n  id: totalAmps\n  id: totalWatts\n  id: totalEnergyDaily\n"""
    assert count_key(good, "friendly_name") == 1
    assert count_key(good + "  friendly_name: duplicate\n", "friendly_name") == 2


def main():
    if "--self-test" in sys.argv:
        self_test()
        return
    paths = sorted(ESP.glob("6chan_energy_meter*.yaml"))
    if {p.name for p in paths} != EXPECTED:
        raise SystemExit("no official top-level meter configurations found")
    validate(paths)
    if {p.name for p in STATUS.glob("6chan_*_status.yaml")} != EXPECTED_STATUS:
        raise AssertionError("status package inventory mismatch")
    print(f"validated {len(paths)} official configs and {len(EXPECTED_STATUS)} status packages")


if __name__ == "__main__":
    main()
