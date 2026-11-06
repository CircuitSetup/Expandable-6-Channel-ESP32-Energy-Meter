#!/bin/bash
set -eu
cd -- "$(dirname -- "$0")"

cat jquery.min.js knockout.min.js knockout.mapping.min.js > ../../src/data/lib.js
