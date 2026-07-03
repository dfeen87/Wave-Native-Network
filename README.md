# Wave-Native Network (WNN)

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Build: C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)]()
[![CI](https://github.com/dfeen87/Wave-Native-Network/actions/workflows/ci.yml/badge.svg)](https://github.com/dfeen87/Wave-Native-Network/actions)

> **A Deterministic Phase-Coherence Overlay for Edge Synchronization and Handoff-Stable Routing**

The Wave-Native Network (WNN) is an experimental deterministic phase-coherence overlay designed for edge synchronization and handoff-stable routing. It augments conventional network transport with oscillator-state tracking and stability-scored diagnostics. WNN is presented strictly as an engineering-level overlay and is **not** a replacement for IP routing, TCP, QUIC, TLS, or cryptographic authentication.

---

## Architecture Overview

WNN treats established carrier infrastructure as a medium for a deeper, wave-driven synchronization layer.

### 1. Node Heartbeat

Each node maintains a deterministic oscillator heartbeat using a Duffing-type nonlinear equation:

$$\dot{x}+0.3\dot{x}-1.0x+1.0x^{3}=0.5\cos(\omega t)$$

The local phase-memory state is represented by a low-dimensional deterministic state vector:

$$\Psi_{i}(t)=\langle A_{i}(t),\theta_{i}(t),\omega_{i}(t),\dot{x}_{i}(t),t_{s}\rangle$$

This vector provides a compact deterministic state against which network synchronization, drift, and handoff continuity can be dynamically scored.

### 2. Phase-Aware Routing Metric

WNN augments standard routing cost logic by incorporating phase-state diagnostics into a composite hybrid metric:

$$d_{WNN}=w_{L}L+w_{J}J+w_{\phi}|\Delta\phi|+w_{\omega}|\Delta\omega|+w_{\Delta\Phi}\Delta\Phi$$

This overlays traditional network latency ($L$) and jitter ($J$) with phase mismatch ($|\Delta\phi|$), frequency mismatch ($|\Delta\omega|$), and a scalar stability divergence ($\Delta\Phi$).

### 3. Phase Salting and Replay-Sensitive Diagnostics

The prototype includes a deterministic perturbation tied to a shared epoch to act as a replay-sensitive diagnostic layer:

$$\delta_{salt}(t_{s})=(Hash(\lfloor\frac{t_{s}}{\Delta E}\rfloor\oplus\omega^{*})\pmod{1000})10^{-5}$$

This is treated as an anomaly-scoring channel, not as a cryptographic primitive.

---

## Empirical Benchmarks & Simulation Results

The repository includes a reproducible benchmarking suite (located in `/simulations/`) built to evaluate the overlay's performance. Current empirical highlights include:

* **Handoff Continuity:** During a simulated moving-node handoff event, the baseline continuity score drops from a pre-event mean of $0.945$ to a post-event mean of $0.484$. The WNN overlay absorbs the disruption, maintaining a highly stable post-event mean of $0.950$.


* **Jitter & Loss Robustness:** Across paired robustness runs, WNN reduces the mean phase error by $34.91\%\pm0.73\%$ relative to the baseline.


* **Scalability:** The overlay degrades gracefully up to 500 simulated nodes, maintaining a $0\%$ sync failure probability under tested parameters.


* **Component Ablation:** Disabling internal WNN modules induces measurable recovery penalties. Disabling the lock detector increases the mean recovery time from $5333.5$ ms to $7243.5$ ms.


* **Replay Detection (Limitation):** The $\Delta\Phi$ anomaly score acts as an experimental replay-sensitive heuristic. At the current fixed threshold, it achieves $76.57\%$ accuracy but only a $40.88\%$ recall and a $20.12\%$ false-positive rate, which remains an active area of optimization.



---

## Reproducibility

To reproduce the exact CSV logs and figures used in the WNN manuscript:

1. Ensure `CMake`, a C++20 compiler, and Python 3 (with `pandas`, `matplotlib`, and `seaborn`) are installed.
2. Navigate to the `/simulations/` directory.
3. Execute the automated data generation and plotting pipeline by running `./run_pipeline.sh`.
4. Raw data matrices will output to `/data/` and 300 DPI visualizations will output to `/results/`.

---

## Build & Installation

WNN is designed for bare-metal execution and requires a modern C++20 compiler to handle the mathematical vectorization required for localized edge hardware.

**Prerequisites:**

* CMake >= 3.20
* `libpcap-dev`
* `gcc` or `clang` with C++20 support

**Build Instructions:**

```bash
git clone https://github.com/dfeen87/Wave-Native-Network.git
cd Wave-Native-Network
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

```

## Configuration & Execution

WNN requires root privileges to bind the `phy_interceptor` directly to the network interface.

```bash
sudo ./wavefrontd --interface eth0 --safety=strict

```

**Safety Modes:**

* `--safety=strict` : Immediate routing refraction upon detecting anomalous metrics.
* `--safety=relaxed` : Tolerates high-latency phase-shimmer.
* `--safety=off` : Bypasses safety routing triggers entirely (Laboratory testing only).

---

## Acknowledgements

I extend my deepest gratitude to Marcel Krüger, Independent Theoretical Physics Researcher. His pioneering work in Helix–Light–Vortex (HLV) dynamics and his original formulation of the Spiral-Time conceptual framework provided a critical theoretical catalyst for this project. The Spiral-Time mathematics and conceptual framework remain his exclusive intellectual property; its application here as a conceptual phase-memory terminology is deeply indebted to his original insights and collaborative spirit.

I also wish to acknowledge Google for providing the ecosystem of advanced computational tools, infrastructure, and AI-driven environments that accelerated the development of this repository.

## License

This project is licensed under the standard **MIT License**. See the `LICENSE` file for details.
