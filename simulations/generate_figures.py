#!/usr/bin/env python3
from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd
import seaborn as sns
from matplotlib.ticker import ScalarFormatter

ROOT = Path(__file__).resolve().parent.parent
DATA_DIR = ROOT / "data"
RESULTS_DIR = ROOT / "results"
RESULTS_DIR.mkdir(parents=True, exist_ok=True)

sns.set_theme(style="whitegrid", context="talk")


def save_fig(fig, name: str):
    fig.tight_layout()
    fig.savefig(RESULTS_DIR / name, dpi=300)
    plt.close(fig)


def figure_phase_convergence():
    df = pd.read_csv(DATA_DIR / "phase_locking.csv")
    agg = (
        df.groupby(["mode", "time_ms"], as_index=False)["phase_error_rad"]
        .mean()
        .assign(abs_phase_error=lambda x: x["phase_error_rad"].abs())
    )

    fig, ax = plt.subplots(figsize=(11, 6))
    sns.lineplot(data=agg, x="time_ms", y="abs_phase_error", hue="mode", ax=ax)
    ax.axhline(0.10, color="black", linestyle="--", linewidth=1.2, label="Stable threshold")
    ax.set_title("Figure 1: True Phase Convergence")
    ax.set_xlabel("Time (ms)")
    ax.set_ylabel("|Phase Error| (rad)")
    ax.legend()
    save_fig(fig, "figure_1_true_phase_convergence.png")


def figure_replay_detection():
    df = pd.read_csv(DATA_DIR / "replay_attack.csv")
    threshold = 0.62

    fig, ax = plt.subplots(figsize=(11, 6))
    sns.scatterplot(
        data=df,
        x="packet_delay_ms",
        y="normalized_delta_phi",
        hue="is_replay",
        style="detected_by_threshold",
        alpha=0.6,
        s=30,
        ax=ax,
    )
    ax.axhline(threshold, color="black", linestyle="--", linewidth=1.2, label="Detection threshold")
    ax.set_ylim(0.0, 1.0)
    ax.set_title("Figure 2: Replay Attack Detection")
    ax.set_xlabel("Packet Delay (ms)")
    ax.set_ylabel("Normalized DeltaPhi")
    ax.xaxis.set_major_formatter(ScalarFormatter(useOffset=False))
    ax.yaxis.set_major_formatter(ScalarFormatter(useOffset=False))
    ax.ticklabel_format(style="plain", axis="both", useOffset=False)
    ax.legend()
    save_fig(fig, "figure_2_replay_attack_detection.png")


def figure_handoff_continuity():
    df = pd.read_csv(DATA_DIR / "handoff_continuity.csv")
    handoff_time = df.loc[df["handoff_event_bool"] == 1, "time_ms"]
    handoff_time = float(handoff_time.iloc[0]) if not handoff_time.empty else 30000.0

    fig, ax = plt.subplots(figsize=(11, 6))
    sns.lineplot(data=df, x="time_ms", y="continuity_score", hue="mode", ax=ax)
    ax.axvline(handoff_time, color="black", linestyle="--", linewidth=1.2, label="Handoff event")
    ax.set_title("Figure 3: Handoff Continuity")
    ax.set_xlabel("Time (ms)")
    ax.set_ylabel("Continuity Score")
    ax.set_ylim(0.0, 1.0)
    ax.legend()
    save_fig(fig, "figure_3_handoff_continuity.png")


def figure_scalability_ablation():
    scalability = pd.read_csv(DATA_DIR / "scalability.csv")
    ablation = pd.read_csv(DATA_DIR / "ablation_study.csv")

    fig, axes = plt.subplots(1, 2, figsize=(15, 6))
    sns.lineplot(
        data=scalability,
        x="node_count",
        y="convergence_time_ms",
        estimator="mean",
        errorbar=("ci", 95),
        marker="o",
        ax=axes[0],
    )
    axes[0].set_title("Scalability Convergence")
    axes[0].set_xlabel("Node Count")
    axes[0].set_ylabel("Convergence Time (ms)")

    sns.barplot(
        data=ablation,
        x="disabled_component",
        y="recovery_time_ms",
        estimator="mean",
        errorbar=("ci", 95),
        ax=axes[1],
    )
    axes[1].set_title("Ablation Recovery Impact")
    axes[1].set_xlabel("Disabled Component")
    axes[1].set_ylabel("Recovery Time (ms)")
    axes[1].tick_params(axis="x", rotation=30)

    fig.suptitle("Figure 4: Scalability & Ablation", y=1.02)
    save_fig(fig, "figure_4_scalability_ablation.png")


def main():
    figure_phase_convergence()
    figure_replay_detection()
    figure_handoff_continuity()
    figure_scalability_ablation()
    print(f"Saved figures to {RESULTS_DIR}")


if __name__ == "__main__":
    main()
