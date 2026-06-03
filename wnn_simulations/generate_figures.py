#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import sys

import matplotlib.pyplot as plt
import pandas as pd
import seaborn as sns


BASE_DIR = Path(__file__).resolve().parent
FIGURES_DIR = BASE_DIR / "figures"


def _load_csv(csv_path: Path) -> pd.DataFrame | None:
    try:
        return pd.read_csv(csv_path)
    except FileNotFoundError:
        print(f"[WARN] Missing CSV: {csv_path}")
        return None
    except Exception as exc:
        print(f"[WARN] Failed to read {csv_path}: {exc}")
        return None


def _save_current_figure(filename: str) -> None:
    out_path = FIGURES_DIR / filename
    plt.tight_layout()
    plt.savefig(out_path, dpi=300, bbox_inches="tight")
    plt.close()
    print(f"[INFO] Saved {out_path}")


def _apply_style() -> None:
    sns.set_theme(style="whitegrid", context="paper")
    plt.rcParams.update(
        {
            "figure.figsize": (7.0, 4.2),
            "axes.facecolor": "white",
            "axes.edgecolor": "#333333",
            "grid.color": "#D9D9D9",
            "grid.linestyle": "-",
            "grid.linewidth": 0.6,
            "axes.labelsize": 11,
            "axes.titlesize": 12,
            "legend.fontsize": 9,
            "font.size": 10,
        }
    )


def plot_phase_convergence() -> None:
    df = _load_csv(BASE_DIR / "convergence_log.csv")
    if df is None or df.empty:
        return

    data = (
        df.groupby("timestamp_ms", as_index=False)["phase_diff_rads"]
        .mean()
        .sort_values("timestamp_ms")
    )

    plt.figure()
    sns.lineplot(data=data, x="timestamp_ms", y="phase_diff_rads", color="#4C72B0", linewidth=2.0)
    plt.xlabel("Timestamp (ms)")
    plt.ylabel("Phase Difference (rad)")
    plt.title("Figure 1. Phase Convergence")
    _save_current_figure("figure_1_phase_convergence.png")


def plot_replay_attack() -> None:
    df = _load_csv(BASE_DIR / "security_log.csv")
    if df is None or df.empty:
        return

    truthy = {"true", "1", "yes", "y", "t"}
    is_attack = (
        df["is_replay_attack"]
        .astype(str)
        .str.strip()
        .str.lower()
        .isin(truthy)
    )
    df = df.assign(replay_attack=is_attack)

    plt.figure()
    sns.scatterplot(
        data=df,
        x="timestamp_ms",
        y="delta_phi_score",
        hue="replay_attack",
        palette={False: "#55A868", True: "#C44E52"},
        s=26,
        edgecolor="none",
    )
    plt.xlabel("Timestamp (ms)")
    plt.ylabel("Δφ Score")
    plt.title("Figure 2. Replay Attack Detection")
    plt.legend(title="Replay Attack", labels=["No", "Yes"])
    _save_current_figure("figure_2_replay_attack.png")


def plot_handoff_continuity() -> None:
    df = _load_csv(BASE_DIR / "handoff_log.csv")
    if df is None or df.empty:
        return

    data = (
        df.groupby("timestamp_ms", as_index=False)["continuity_score"]
        .mean()
        .sort_values("timestamp_ms")
    )

    plt.figure()
    sns.lineplot(data=data, x="timestamp_ms", y="continuity_score", color="#8172B3", linewidth=2.0)
    plt.xlabel("Timestamp (ms)")
    plt.ylabel("Continuity Score")
    plt.title("Figure 3. Handoff Continuity")
    _save_current_figure("figure_3_handoff_continuity.png")


def plot_scalability_matrix() -> None:
    df = _load_csv(BASE_DIR / "scalability_log.csv")
    if df is None or df.empty:
        return

    mode_map = {
        "baseline": "Baseline",
        "wnn_overlay": "WNN Overlay",
        "ablation_no_delta_phi": "Ablation",
    }
    filtered = df[df["mode"].isin(mode_map.keys())].copy()
    if filtered.empty:
        print("[WARN] No baseline/overlay/ablation data found in scalability_log.csv")
        return

    filtered["mode_label"] = filtered["mode"].map(mode_map)

    plt.figure()
    sns.barplot(
        data=filtered,
        x="node_count",
        y="mean_time_between_sync_failures",
        hue="mode_label",
        palette="muted",
        errorbar=None,
    )
    plt.xlabel("Node Count")
    plt.ylabel("Mean Time Between Sync Failures (ms)")
    plt.title("Figure 4. Scalability Matrix")
    plt.legend(title="Mode")
    _save_current_figure("figure_4_scalability_matrix.png")


def main() -> int:
    FIGURES_DIR.mkdir(parents=True, exist_ok=True)
    _apply_style()

    plot_phase_convergence()
    plot_replay_attack()
    plot_handoff_continuity()
    plot_scalability_matrix()
    return 0


if __name__ == "__main__":
    sys.exit(main())
