#!/usr/bin/env python3
from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd
import seaborn as sns
from matplotlib.lines import Line2D
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


def apply_plain_axis_format(ax):
    ax.xaxis.set_major_formatter(ScalarFormatter(useOffset=False))
    ax.yaxis.set_major_formatter(ScalarFormatter(useOffset=False))
    ax.ticklabel_format(style="plain", axis="both", useOffset=False)


def average_lock_window_duration_ms(series: pd.Series, threshold: float, dt_ms: float) -> float:
    lock_durations = []
    current = 0
    for value in series:
        if value < threshold:
            current += 1
        elif current:
            lock_durations.append(current * dt_ms)
            current = 0
    if current:
        lock_durations.append(current * dt_ms)
    if not lock_durations:
        return 0.0
    return float(sum(lock_durations) / len(lock_durations))


def figure_phase_tracking():
    df = pd.read_csv(DATA_DIR / "phase_locking.csv")
    threshold = 0.10
    agg = (
        df.groupby(["mode", "time_ms"], as_index=False)["phase_error_rad"]
        .mean()
        .assign(abs_phase_error=lambda x: x["phase_error_rad"].abs())
    )
    dt_ms = float(agg.groupby("mode")["time_ms"].diff().dropna().median())
    if pd.isna(dt_ms) or dt_ms <= 0:
        dt_ms = 100.0

    fig, ax = plt.subplots(figsize=(11, 6))
    sns.lineplot(data=agg, x="time_ms", y="abs_phase_error", hue="mode", ax=ax)
    ax.axhline(threshold, color="black", linestyle="--", linewidth=1.2, label="Threshold")
    ax.set_title("Figure 1: Phase-State Tracking and Oscillatory Re-locking")
    ax.set_xlabel("Time (ms)")
    ax.set_ylabel("|Phase Error| (rad)")
    ax.legend()
    apply_plain_axis_format(ax)
    save_fig(fig, "figure_1_true_phase_convergence.png")

    for mode, mode_df in agg.groupby("mode"):
        series = mode_df.sort_values("time_ms")["abs_phase_error"]
        time_below_ms = float((series < threshold).sum() * dt_ms)
        mean_phase_error = float(series.mean())
        avg_lock_window_ms = average_lock_window_duration_ms(series, threshold, dt_ms)
        print(
            f"{mode} - Time Below Threshold: {time_below_ms:.2f} ms, "
            f"Mean Phase Error: {mean_phase_error:.6f}, "
            f"Average Lock-Window Duration: {avg_lock_window_ms:.2f} ms"
        )


def figure_replay_detection():
    df = pd.read_csv(DATA_DIR / "replay_attack.csv")
    threshold = 0.62
    label_map = {0: "Normal", 1: "Replay"}
    plot_df = df.copy()
    plot_df["class"] = plot_df["is_replay"].map(label_map)
    plot_df["predicted_replay"] = (plot_df["normalized_delta_phi"] >= threshold).astype(int)

    tp = int(((plot_df["predicted_replay"] == 1) & (plot_df["is_replay"] == 1)).sum())
    tn = int(((plot_df["predicted_replay"] == 0) & (plot_df["is_replay"] == 0)).sum())
    fp = int(((plot_df["predicted_replay"] == 1) & (plot_df["is_replay"] == 0)).sum())
    fn = int(((plot_df["predicted_replay"] == 0) & (plot_df["is_replay"] == 1)).sum())

    tpr = tp / (tp + fn) if (tp + fn) else 0.0
    fpr = fp / (fp + tn) if (fp + tn) else 0.0
    accuracy = (tp + tn) / len(plot_df) if len(plot_df) else 0.0

    fig, ax = plt.subplots(figsize=(11, 6))
    sns.scatterplot(
        data=plot_df,
        x="packet_delay_ms",
        y="normalized_delta_phi",
        hue="class",
        hue_order=["Normal", "Replay"],
        alpha=0.6,
        s=30,
        ax=ax,
    )
    ax.axhline(threshold, color="black", linestyle="--", linewidth=1.2)
    ax.set_ylim(0.0, 1.0)
    ax.set_title("Figure 2: Replay Attack Detection")
    ax.set_xlabel("Packet Delay (ms)")
    ax.set_ylabel("Normalized DeltaPhi")
    apply_plain_axis_format(ax)

    palette = sns.color_palette()
    legend_handles = [
        Line2D([0], [0], marker="o", linestyle="None", markersize=8, markerfacecolor=palette[0], markeredgecolor=palette[0], label="Normal"),
        Line2D([0], [0], marker="o", linestyle="None", markersize=8, markerfacecolor=palette[1], markeredgecolor=palette[1], label="Replay"),
        Line2D([0], [0], color="black", linestyle="--", linewidth=1.2, label="Threshold"),
    ]
    ax.legend(handles=legend_handles, labels=["Normal", "Replay", "Threshold"], title=None)
    save_fig(fig, "figure_2_replay_attack_detection.png")

    stats_path = RESULTS_DIR / "replay_statistics.txt"
    with stats_path.open("w", encoding="utf-8") as f:
        f.write("Replay Detection Statistics\n")
        f.write(f"Threshold: {threshold:.3f}\n")
        f.write(f"True Positive Rate (TPR): {tpr:.6f}\n")
        f.write(f"False Positive Rate (FPR): {fpr:.6f}\n")
        f.write(f"Detection Accuracy: {accuracy:.6f}\n")
        f.write("Confusion Matrix (Actual x Predicted)\n")
        f.write("               Predicted Normal  Predicted Replay\n")
        f.write(f"Actual Normal   {tn:16d}  {fp:16d}\n")
        f.write(f"Actual Replay   {fn:16d}  {tp:16d}\n")


def figure_handoff_continuity():
    df = pd.read_csv(DATA_DIR / "handoff_continuity.csv")
    handoff_time = df.loc[df["handoff_event_bool"] == 1, "time_ms"]
    handoff_time = float(handoff_time.iloc[0]) if not handoff_time.empty else 30000.0

    fig, ax = plt.subplots(figsize=(11, 6))
    sns.lineplot(data=df, x="time_ms", y="continuity_score", hue="mode", ax=ax)
    ax.axvline(handoff_time, color="black", linestyle="--", linewidth=1.2, label="Handoff event")
    ax.set_title("Figure 3: Handoff Continuity and Recovery")
    ax.set_xlabel("Time (ms)")
    ax.set_ylabel("Continuity Score")
    ax.set_ylim(0.0, 1.0)
    ax.legend()
    apply_plain_axis_format(ax)
    save_fig(fig, "figure_3_handoff_continuity.png")


def figure_scalability_ablation():
    scalability = pd.read_csv(DATA_DIR / "scalability.csv")
    ablation = pd.read_csv(DATA_DIR / "ablation_study.csv")

    fig, axes = plt.subplots(1, 2, figsize=(15, 6))
    sns.lineplot(
        data=scalability,
        x="node_count",
        y="routing_stability_score",
        marker="o",
        ax=axes[0],
    )
    axes[0].set_title("Scalability Routing Stability")
    axes[0].set_xlabel("Node Count")
    axes[0].set_ylabel("Routing Stability Score")
    apply_plain_axis_format(axes[0])

    sns.barplot(
        data=ablation,
        x="disabled_component",
        y="recovery_time_ms",
        estimator="mean",
        errorbar="sd",
        ax=axes[1],
    )
    axes[1].set_title("Ablation Recovery Impact")
    axes[1].set_xlabel("Disabled Component")
    axes[1].set_ylabel("Recovery Time (ms)")
    axes[1].tick_params(axis="x", rotation=30)
    apply_plain_axis_format(axes[1])

    fig.suptitle("Figure 4: Scalability & Ablation", y=1.02)
    save_fig(fig, "figure_4_scalability_ablation.png")


def main():
    figure_phase_tracking()
    figure_replay_detection()
    figure_handoff_continuity()
    figure_scalability_ablation()
    print(f"Saved figures to {RESULTS_DIR}")


if __name__ == "__main__":
    main()
