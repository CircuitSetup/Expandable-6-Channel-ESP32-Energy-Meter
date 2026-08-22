import re
from pathlib import Path


def test_release_actions_are_pinned_to_commits() -> None:
    workflow = (Path(__file__).parents[3] / ".github/workflows/esphome-compile.yml").read_text()
    actions = re.findall(r"uses:\s*([^\s#]+)", workflow)

    assert actions
    assert all(re.fullmatch(r"[^@]+@[0-9a-f]{40}", action) for action in actions), actions
