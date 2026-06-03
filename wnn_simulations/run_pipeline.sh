#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}"
cmake --build "${BUILD_DIR}" -j"$(nproc)"
"${BUILD_DIR}/wnn_sim_runner"
python3 "${SCRIPT_DIR}/generate_figures.py"
