# Task 6 report

- RED: `python scripts/validate_helper_contract.py` was unavailable before implementation; the new `--self-test` initially failed until the line matcher was corrected.
- GREEN: `python scripts/validate_helper_contract.py --self-test` passed.
- Live validation passed: 22 official top-level `Software/ESPHome/6chan_energy_meter*.yaml` files (examples excluded) and all 7 status packages.
- CI: `.github/workflows/esphome-compile.yml` invokes `python scripts/validate_helper_contract.py` after Python setup and before ESPHome installation/compile matrix execution.
- Contract checks: contract version, required substitution scalars, active CT calibration pairs, optional package multiplicity, totals for add-on configs, forbidden `board_revision`, and diagnostic/disabled status entities.
- Syntax/quality: `python -m py_compile scripts/validate_helper_contract.py` and `git diff --check` passed.
- Representative compile command/coverage: existing workflow runs `pip install esphome==2026.8.0` then `esphome compile ${{ matrix.configurations }}`; this task did not rerun the network-dependent matrix locally.
- Limitation: main-only totals are supplied by their included shared package, while add-on totals are asserted directly in each official add-on config.
