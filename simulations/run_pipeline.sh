#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

mkdir -p "${BUILD_DIR}"
cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
cmake --build "${BUILD_DIR}" -j"$(nproc)"

"${BUILD_DIR}/simulations/run_phase_locking"
"${BUILD_DIR}/simulations/run_jitter_robustness"
"${BUILD_DIR}/simulations/run_handoff"
"${BUILD_DIR}/simulations/run_replay_attack"
"${BUILD_DIR}/simulations/run_ablation"
"${BUILD_DIR}/simulations/run_scalability"

python3 "${ROOT_DIR}/simulations/generate_figures.py"

echo "Pipeline complete. CSVs in ${ROOT_DIR}/data and figures in ${ROOT_DIR}/results"
