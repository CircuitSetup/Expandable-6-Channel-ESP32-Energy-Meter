"""Dependency-free contract checks for official ESPHome meter configurations."""
from pathlib import Path
import re
import sys
import tempfile

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

def mapping_keys(text, section="substitutions"):
    rows = text.splitlines(); start = next((i for i, x in enumerate(rows) if re.match(rf"^\s*{section}:\s*$", x)), None)
    if start is None: return {}
    base = len(rows[start]) - len(rows[start].lstrip()); out = {}
    for line in rows[start + 1:]:
        if line.strip() and not line.lstrip().startswith("#"):
            indent = len(line) - len(line.lstrip())
            if indent <= base: break
            match = re.match(r"^(\s*)([A-Za-z0-9_]+)\s*:", line)
            if match and len(match.group(1)) == base + 2: out.setdefault(match.group(2), 0); out[match.group(2)] += 1
    return out


def validate_config(path):
    text = "\n".join(lines(path))
    substitutions = mapping_keys(text)
    errors = []
    if count_key(text, "csemh_config_contract") != 1 or not re.search(r'^[ \t]*csemh_config_contract[ \t]*:[ \t]*["\']2["\'][ \t]*$', text, re.M):
        errors.append('csemh_config_contract must be "2" exactly once')
    for key in REQUIRED:
        if substitutions.get(key, 0) != 1:
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
    required_totals = ("totalAmps", "totalWatts", "totalEnergyDaily") if "addon" in path.stem else ("totalEnergyDaily",)
    for key in required_totals:
        present = len(re.findall(rf"^[ \t]+id:[ \t]*{key}[ \t]*$", text, re.M)) == 1
        if not present:
            errors.append(f"{key} must have one definition")
    return errors


def validate_status():
    errors = []
    for path in sorted(STATUS.glob("6chan_*_status.yaml")):
        errors.extend(validate_status_file(path))
    return errors

def validate_status_file(path):
    errors = []
    text = "\n".join(lines(path))
    rows = text.splitlines(); names = [(i, len(x) - len(x.lstrip())) for i, x in enumerate(rows) if re.match(r"^[ \t]+name:\s*", x) and not x.lstrip().startswith("#")]
    valid = all(any(len(rows[j]) - len(rows[j].lstrip()) == indent and rows[j].strip() == "entity_category: diagnostic" for j in range(i + 1, next((k for k in range(i + 1, len(rows)) if rows[k].strip() and len(rows[k]) - len(rows[k].lstrip()) < indent), len(rows)))) and any(len(rows[j]) - len(rows[j].lstrip()) == indent and rows[j].strip() == "disabled_by_default: true" for j in range(i + 1, next((k for k in range(i + 1, len(rows)) if rows[k].strip() and len(rows[k]) - len(rows[k].lstrip()) < indent), len(rows)))) for i, indent in names)
    if not names or not valid:
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
    assert mapping_keys(good)["friendly_name"] == 1
    bad = good.replace("  friendly_name: Meter", "wifi:\n    friendly_name: Meter")
    assert mapping_keys(bad).get("friendly_name", 0) == 0
    with tempfile.TemporaryDirectory() as directory:
        fixture = Path(directory) / "6chan_energy_meter_1-addon.yaml"
        source = ESP / "6chan_energy_meter_1-addon.yaml"
        fixture.write_text(source.read_text(encoding="utf-8"), encoding="utf-8")
        assert not validate_config(fixture), validate_config(fixture)
        fixture.write_text(fixture.read_text(encoding="utf-8").replace("  id: totalAmps\n", ""), encoding="utf-8")
        assert "totalAmps must have one definition" in validate_config(fixture)
        duplicate = fixture.with_name("6chan_energy_meter_2-addons.yaml")
        duplicate.write_text(source.read_text(encoding="utf-8").replace("6chan_main_status.yaml", "6chan_main_status.yaml\n      - Software/ESPHome/status_fields/6chan_main_status.yaml"), encoding="utf-8")
        assert any("duplicate package include" in e for e in validate_config(duplicate))
        distinct = fixture.with_name("6chan_energy_meter_3-addons.yaml")
        distinct.write_text(source.read_text(encoding="utf-8").replace("6chan_main_status.yaml", "6chan_main_status.yaml\n      - Software/ESPHome/status_fields/6chan_addon1_status.yaml"), encoding="utf-8")
        assert not any("duplicate package include" in e for e in validate_config(distinct))
        status = fixture.with_name("6chan_main_status.yaml")
        valid_status = "text_sensor:\n  - platform: atm90e32\n    phase_status:\n      phase_a:\n        name: Status\n        entity_category: diagnostic\n        disabled_by_default: true\n"
        status.write_text(valid_status, encoding="utf-8")
        assert not validate_status_file(status)
        moved = fixture.with_name("6chan_energy_meter_moved.yaml")
        moved.write_text(source.read_text(encoding="utf-8").replace('  friendly_name: "CircuitSetup Energy Meter 12x"\n', "wifi:\n  friendly_name: misplaced\n"), encoding="utf-8")
        assert "friendly_name must exist exactly once" in validate_config(moved)
        status.write_text("""text_sensor:\n  - platform: atm90e32\n    phase_status:\n      phase_a:\n        name: Missing flags\n      phase_b:\n        name: Valid\n        entity_category: diagnostic\n        disabled_by_default: true\n""", encoding="utf-8")
        assert validate_status_file(status)
        status.write_text("""text_sensor:\n  - platform: atm90e32\n    phase_status:\n      phase_a:\n        name: Unrelated flags\n      other:\n        entity_category: diagnostic\n        disabled_by_default: true\n""", encoding="utf-8")
        assert validate_status_file(status)
        status.write_text(valid_status.replace("entity_category: diagnostic", "# entity_category: diagnostic"), encoding="utf-8")
        assert validate_status_file(status)
        status.write_text(valid_status.replace("disabled_by_default: true", "# disabled_by_default: true"), encoding="utf-8")
        assert validate_status_file(status)


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
