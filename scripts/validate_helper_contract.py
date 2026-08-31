"""Dependency-free contract checks for official ESPHome meter configurations."""

from collections import Counter
from pathlib import Path
import re
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
ESP = ROOT / "Software" / "ESPHome"
STATUS = ESP / "status_fields"
REQUIRED = ("friendly_name", "update_time", "electric_freq", "voltage_cal1", "voltage_cal2")
EXPECTED = {f"6chan_energy_meter_{n}-{'addon' if n == 1 else 'addons'}{suffix}.yaml" for n in range(1, 7) for suffix in ("", "_ethernet", "_ethernet_waveshare")} | {"6chan_energy_meter_main_board.yaml", "6chan_energy_meter_main_ethernet.yaml", "6chan_energy_meter_main_ethernet_waveshare.yaml", "6chan_energy_meter_3-addons_2-voltages.yaml"}
EXPECTED_STATUS = {f"6chan_{kind}_status.yaml" for kind in ("main", "addon1", "addon2", "addon3", "addon4", "addon5", "addon6")}


def lines(path):
    return path.read_text(encoding="utf-8").splitlines()


def count_key(text, key):
    return sum(bool(re.match(rf"^[ \t]*{re.escape(key)}[ \t]*:", line)) for line in text.splitlines())


def substitutions(text):
    rows = text.splitlines()
    starts = [index for index, line in enumerate(rows) if re.match(r"^substitutions:[ \t]*$", line)]
    if len(starts) != 1:
        return None
    result = {}
    for line in rows[starts[0] + 1:]:
        if line.strip() and not line.lstrip().startswith("#") and not line.startswith((" ", "\t")):
            break
        match = re.match(r"^  ([A-Za-z0-9_]+)[ \t]*:[ \t]*(.*)$", line)
        if match:
            result.setdefault(match.group(1), []).append(match.group(2).strip())
    return result


def addon_count(path):
    match = re.search(r"_(\d+)-addons?(?:_|\.yaml)", path.name)
    if match:
        return int(match.group(1))
    if path.name.startswith("6chan_energy_meter_main_"):
        return 0
    raise ValueError(f"cannot derive add-on count from {path.name}")


def validate_config(path):
    text = "\n".join(lines(path))
    direct = substitutions(text)
    errors = []
    if direct is None:
        errors.append("substitutions must be one top-level mapping")
        direct = {}
    contract = direct.get("csemh_config_contract", [])
    if len(contract) != 1 or not re.fullmatch(r'(?:"2"|\'2\')', contract[0]):
        errors.append('csemh_config_contract must be "2" exactly once')
    for key in REQUIRED:
        if len(direct.get(key, [])) != 1:
            errors.append(f"{key} must exist exactly once")
    try:
        expected_cts = set(range(1, 6 * (addon_count(path) + 1) + 1))
    except ValueError as error:
        errors.append(str(error))
        expected_cts = set()
    for prefix, suffix in (("ct", "_name"), ("current_cal_ct", "")):
        found = {int(match.group(1)): len(values) for key, values in direct.items() if (match := re.fullmatch(rf"{prefix}(\d+){suffix}", key))}
        for number in sorted(expected_cts | set(found)):
            key = f"{prefix}{number}{suffix}"
            if number not in expected_cts or found.get(number) != 1:
                errors.append(f"{key} must exist exactly once")
    if count_key(text, "board_revision"):
        errors.append("board_revision is not permitted")
    includes = re.findall(r"^[ \t]*- (Software/ESPHome/(?:power_quality|status_fields)/[^\s#]+\.yaml)[ \t]*$", text, re.M)
    for include in set(includes):
        if includes.count(include) > 1:
            errors.append(f"duplicate package include: {include}")
    required_totals = ("totalAmps", "totalWatts", "totalEnergyDaily") if "addon" in path.stem else ("totalEnergyDaily",)
    for key in required_totals:
        if len(re.findall(rf"^[ \t]+id:[ \t]*{key}[ \t]*$", text, re.M)) != 1:
            errors.append(f"{key} must have one definition")
    return errors


def entity_flagged(rows, name_index, indent):
    fields = set()
    for line in rows[name_index + 1:]:
        if line.strip() and len(line) - len(line.lstrip()) < indent:
            break
        if len(line) - len(line.lstrip()) == indent:
            fields.add(line.strip())
    return "entity_category: diagnostic" in fields and "disabled_by_default: true" in fields


def status_entities(path):
    rows = lines(path)
    parent = None
    phase = None
    entities = []
    for index, line in enumerate(rows):
        if re.match(r"^  - platform: atm90e32[ \t]*$", line):
            parent = None
            phase = None
        elif match := re.match(r"^    id:[ \t]*(\$\{[^}]+\})[ \t]*$", line):
            parent = match.group(1)
        elif match := re.match(r"^      (phase_[abc]):[ \t]*$", line):
            phase = match.group(1)
        elif re.match(r"^    frequency_status:[ \t]*$", line):
            phase = "frequency_status"
        elif match := re.match(r"^( {6,8})name:[ \t]*(.+?)[ \t]*$", line):
            entities.append((parent, phase, match.group(2), entity_flagged(rows, index, len(match.group(1)))))
    return entities


def expected_status(path):
    if path.name == "6chan_main_status.yaml":
        parents, start, frequency = ("${main_meter_id1}", "${main_meter_id2}"), 1, True
    else:
        match = re.fullmatch(r"6chan_addon([1-6])_status\.yaml", path.name)
        if not match:
            raise ValueError(f"unknown status package: {path.name}")
        addon = int(match.group(1))
        parents, start, frequency = (f"${{addon{addon}_id1}}", f"${{addon{addon}_id2}}"), 6 * addon + 1, False
    entities = []
    for parent, first in zip(parents, (start, start + 3)):
        for phase, number in zip(("phase_a", "phase_b", "phase_c"), range(first, first + 3)):
            entities.append((parent, phase, f'"${{ct{number}_name}} Status"'))
    if frequency:
        entities.append((parents[0], "frequency_status", '"Frequency Status 1"'))
    return parents, entities


def validate_status_file(path):
    actual = status_entities(path)
    parents, expected = expected_status(path)
    errors = []
    parent_counts = Counter(parent for parent, _, _, _ in actual)
    expected_parents = Counter({parent: 3 + int(parent == parents[0] and path.name == "6chan_main_status.yaml") for parent in parents})
    if parent_counts != expected_parents:
        errors.append(f"{path.name}: parent IDs or entity associations do not match inventory")
    if Counter((parent, phase, name) for parent, phase, name, _ in actual) != Counter(expected):
        errors.append(f"{path.name}: status entity inventory does not match")
    if len(actual) != len(expected) or not all(flagged for _, _, _, flagged in actual):
        errors.append(f"{path.name}: every status entity must be diagnostic and disabled_by_default")
    return errors


def validate_status():
    return [error for path in sorted(STATUS.glob("6chan_*_status.yaml")) for error in validate_status_file(path)]


def official_paths():
    if {path.name for path in ESP.glob("6chan_energy_meter*.yaml")} != EXPECTED:
        raise SystemExit("official top-level meter configuration inventory mismatch")
    paths = [ESP / name for name in sorted(EXPECTED)]
    if not all(path.is_file() for path in paths):
        raise SystemExit("no official top-level meter configurations found")
    return paths


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
    assert {path.name for path in official_paths()} == EXPECTED
    with tempfile.TemporaryDirectory() as directory:
        fixture = Path(directory) / "6chan_energy_meter_1-addon.yaml"
        source = ESP / fixture.name
        fixture.write_text(source.read_text(encoding="utf-8"), encoding="utf-8")
        assert not validate_config(fixture), validate_config(fixture)
        fixture.write_text(fixture.read_text(encoding="utf-8").replace("  id: totalAmps\n", ""), encoding="utf-8")
        assert "totalAmps must have one definition" in validate_config(fixture)
        duplicate = fixture.with_name("6chan_energy_meter_2-addons.yaml")
        duplicate.write_text(source.read_text(encoding="utf-8").replace("6chan_main_status.yaml", "6chan_main_status.yaml\n      - Software/ESPHome/status_fields/6chan_main_status.yaml"), encoding="utf-8")
        assert any("duplicate package include" in error for error in validate_config(duplicate))
        distinct = fixture.with_name("6chan_energy_meter_3-addons.yaml")
        distinct.write_text(source.read_text(encoding="utf-8").replace("6chan_main_status.yaml", "6chan_main_status.yaml\n      - Software/ESPHome/status_fields/6chan_addon1_status.yaml"), encoding="utf-8")
        assert not any("duplicate package include" in error for error in validate_config(distinct))
        moved = fixture.with_name("6chan_energy_meter_moved.yaml")
        moved.write_text(source.read_text(encoding="utf-8").replace('  friendly_name: "CircuitSetup Energy Meter 12x"\n', "wifi:\n  friendly_name: misplaced\n"), encoding="utf-8")
        assert "friendly_name must exist exactly once" in validate_config(moved)
        nested_contract = fixture.with_name("6chan_energy_meter_nested.yaml")
        nested_contract.write_text(source.read_text(encoding="utf-8").replace('  csemh_config_contract: "2"\n', "wifi:\n  csemh_config_contract: \"2\"\n"), encoding="utf-8")
        assert 'csemh_config_contract must be "2" exactly once' in validate_config(nested_contract)
        unquoted_contract = fixture.with_name("6chan_energy_meter_unquoted.yaml")
        unquoted_contract.write_text(source.read_text(encoding="utf-8").replace('  csemh_config_contract: "2"', "  csemh_config_contract: 2"), encoding="utf-8")
        assert 'csemh_config_contract must be "2" exactly once' in validate_config(unquoted_contract)
        single_quoted_contract = fixture.with_name("6chan_energy_meter_1-addon_single_quoted.yaml")
        single_quoted_contract.write_text(source.read_text(encoding="utf-8").replace('  csemh_config_contract: "2"', "  csemh_config_contract: '2'"), encoding="utf-8")
        assert not validate_config(single_quoted_contract), validate_config(single_quoted_contract)
        mismatched_contract = fixture.with_name("6chan_energy_meter_1-addon_mismatched.yaml")
        mismatched_contract.write_text(source.read_text(encoding="utf-8").replace('  csemh_config_contract: "2"', "  csemh_config_contract: \"2'"), encoding="utf-8")
        assert 'csemh_config_contract must be "2" exactly once' in validate_config(mismatched_contract)
        duplicate_substitutions = fixture.with_name("6chan_energy_meter_duplicate.yaml")
        duplicate_substitutions.write_text(source.read_text(encoding="utf-8") + "\nsubstitutions:\n  csemh_config_contract: \"2\"\n", encoding="utf-8")
        assert "substitutions must be one top-level mapping" in validate_config(duplicate_substitutions)
        source_text = source.read_text(encoding="utf-8")
        start, end = source_text.index("substitutions:\n"), source_text.index("\nesphome:")
        nested_substitutions = fixture.with_name("6chan_energy_meter_nested_substitutions.yaml")
        nested_substitutions.write_text(source_text[:start] + "wifi:\n  " + source_text[start:end].replace("\n", "\n  ") + source_text[end:], encoding="utf-8")
        assert "substitutions must be one top-level mapping" in validate_config(nested_substitutions)
        missing_pair = fixture.with_name("6chan_energy_meter_1-addon_missing_pair.yaml")
        missing_pair.write_text(source.read_text(encoding="utf-8").replace("  ct12_name: CT12\n", "").replace("  current_cal_ct12: '27518'\n", ""), encoding="utf-8")
        assert any("ct12_name" in error for error in validate_config(missing_pair))
        extra_pair = fixture.with_name("6chan_energy_meter_1-addon_extra_pair.yaml")
        extra_pair.write_text(source.read_text(encoding="utf-8").replace("  ct12_name: CT12\n", "  ct12_name: CT12\n  ct13_name: CT13\n").replace("  current_cal_ct12: '27518'\n", "  current_cal_ct12: '27518'\n  current_cal_ct13: '27518'\n"), encoding="utf-8")
        assert any("ct13_name" in error for error in validate_config(extra_pair))
        main_status = STATUS / "6chan_main_status.yaml"
        main_mutation = fixture.with_name("6chan_main_status.yaml")
        main_mutation.write_text(main_status.read_text(encoding="utf-8").replace("id: ${main_meter_id2}", "id: ${wrong_parent}"), encoding="utf-8")
        assert validate_status_file(main_mutation)
        main_mutation.write_text(main_status.read_text(encoding="utf-8").replace('      phase_c:\n        name: "${ct6_name} Status"\n        entity_category: diagnostic\n        disabled_by_default: true\n', ""), encoding="utf-8")
        assert validate_status_file(main_mutation)
        main_mutation.write_text(main_status.read_text(encoding="utf-8").replace("disabled_by_default: true", "# disabled_by_default: true", 1), encoding="utf-8")
        assert validate_status_file(main_mutation)
        addon_status = STATUS / "6chan_addon6_status.yaml"
        addon_mutation = fixture.with_name("6chan_addon6_status.yaml")
        addon_mutation.write_text(addon_status.read_text(encoding="utf-8").replace("id: ${addon6_id2}", "id: ${wrong_parent}"), encoding="utf-8")
        assert validate_status_file(addon_mutation)
    extra = ESP / "6chan_energy_meter_7-addons.yaml"
    try:
        extra.write_text("# mutation\n", encoding="utf-8")
        try:
            official_paths()
        except SystemExit:
            pass
        else:
            raise AssertionError("official_paths accepted an extra configuration")
    finally:
        extra.unlink(missing_ok=True)


def main():
    if "--self-test" in sys.argv:
        self_test()
        return
    paths = official_paths()
    if {path.name for path in STATUS.glob("6chan_*_status.yaml")} != EXPECTED_STATUS:
        raise AssertionError("status package inventory mismatch")
    validate(paths)
    print(f"validated {len(paths)} official configs and {len(EXPECTED_STATUS)} status packages")


if __name__ == "__main__":
    main()
