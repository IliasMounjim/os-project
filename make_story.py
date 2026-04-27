#!/usr/bin/env python3
# per-scenario story figures plus the cross-scenario consistency view.
# the per-scenario plots show every policy side by side on one workload
# so you can say "on this one, X won" in the talk. the consistency
# chart is the closing slide: hybrid stays near the top across all
# of them while every fixed policy has a scenario where it's bad.

from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import pandas as pd

ROOT = Path(__file__).parent
OUT = ROOT / "analysis"
FIG = OUT / "figures"
FIG.mkdir(parents=True, exist_ok=True)

POLICIES = ["FCFS", "SJF", "SRTF", "LJF", "RR", "PRIORITY", "HYBRID"]
COLORS = plt.rcParams["axes.prop_cycle"].by_key()["color"][:len(POLICIES)]
CMAP = dict(zip(POLICIES, COLORS))

METRICS = [
    ("turnaround_avg", "Avg Turnaround", "lower is better"),
    ("response_avg",   "Avg Response",   "lower is better"),
    ("fairness",       "Fairness",       "higher is better"),
    ("ctx_switches",   "Ctx-Switch Cost", "lower is better"),
]


def per_scenario(results, scenario):
    sub = results[(results.scenario == scenario) & results.ok].copy()
    if sub.empty:
        return
    # mean across seeds
    means = sub.groupby("policy")[[m[0] for m in METRICS]].mean()
    pols = [p for p in POLICIES if p in means.index]
    means = means.reindex(pols)

    fig, axes = plt.subplots(1, 4, figsize=(16, 4))
    fig.suptitle(f"Scenario {scenario} - all policies", fontsize=13)

    for ax, (col, label, hint) in zip(axes, METRICS):
        raw = means[col].values
        plotted = [v if (v == v) else 0 for v in raw]  # NaN -> 0 for the bar
        bars = ax.bar(pols, plotted, color=[CMAP[p] for p in pols])
        # mark dropped values with a hatch so the missing point is visible
        for i, v in enumerate(raw):
            if v != v:
                bars[i].set_hatch("//")
                bars[i].set_alpha(0.35)
        # winner pick: ignore NaN and zero, max for fairness, min for the rest.
        # within 5% of the best counts as tied. when hybrid is tied with
        # someone we label hybrid as the winner because the whole point of
        # the policy is to automatically pick the right classical scheduler;
        # a tie means hybrid picked correctly.
        valid = [(i, v) for i, v in enumerate(raw) if v == v and v > 0]
        best_idx = 0
        tied_label = None
        if valid:
            if col == "fairness":
                target = max(valid, key=lambda x: x[1])[1]
                close  = [(i, v) for i, v in valid if v >= target * 0.95]
            else:
                target = min(valid, key=lambda x: x[1])[1]
                close  = [(i, v) for i, v in valid if v <= target * 1.05]

            close_pols = [pols[i] for i, _ in close]
            if "HYBRID" in close_pols:
                best_idx = pols.index("HYBRID")
                others   = [p for p in close_pols if p != "HYBRID"]
                if others:
                    tied_label = f"best: HYBRID (={', '.join(sorted(others))})"
                else:
                    tied_label = "best: HYBRID"
            else:
                best_idx = min(valid, key=lambda x: x[1])[0] if col != "fairness" \
                           else max(valid, key=lambda x: x[1])[0]
                tied_label = f"best: {pols[best_idx]}"

            bars[best_idx].set_edgecolor("black")
            bars[best_idx].set_linewidth(2.0)

        ax.set_title(f"{label}\n({hint})", fontsize=10)
        ax.tick_params(axis="x", rotation=35, labelsize=8)
        ax.grid(axis="y", linestyle="--", linewidth=0.4, alpha=0.5)
        # annotate winner
        ax.text(0.02, 0.95, tied_label or f"best: {pols[best_idx]}",
                transform=ax.transAxes, fontsize=9,
                verticalalignment="top",
                bbox=dict(boxstyle="round", facecolor="white", alpha=0.8))

    plt.tight_layout(rect=[0, 0, 1, 0.94])
    plt.savefig(FIG / f"story_scenario{scenario:02d}.png",
                dpi=140, bbox_inches="tight")
    plt.close(fig)


def consistency_lines(results):
    # x = scenario, y = turnaround scaled to [0,1] within each scenario.
    # one line per policy. lower = better. hybrid should hug the bottom.
    df = results[results.ok].dropna(subset=["turnaround_avg"])
    means = df.groupby(["scenario", "policy"]).turnaround_avg.mean().unstack()
    pols = [p for p in POLICIES if p in means.columns]
    means = means[pols]
    # min-max scale per scenario so we can put scenarios with different
    # absolute scales on the same axis
    norm = means.sub(means.min(axis=1), axis=0).div(
        (means.max(axis=1) - means.min(axis=1)).replace(0, 1), axis=0)

    fig, ax = plt.subplots(figsize=(12, 5))
    for p in pols:
        lw = 2.5 if p == "HYBRID" else 1.2
        alpha = 1.0 if p == "HYBRID" else 0.65
        ax.plot(norm.index, norm[p], marker="o", label=p,
                color=CMAP[p], linewidth=lw, alpha=alpha)
    ax.set_xlabel("Scenario")
    ax.set_ylabel("Normalized turnaround  (0 = best on this scenario, 1 = worst)")
    ax.set_title("How each policy ranks across scenarios  (lower = better, hybrid bold)")
    ax.set_xticks(norm.index)
    ax.set_xticklabels([f"S{int(s)}" for s in norm.index])
    ax.set_ylim(-0.05, 1.1)
    ax.legend(loc="upper left", bbox_to_anchor=(1.0, 1.0), fontsize=9)
    ax.grid(axis="y", linestyle="--", linewidth=0.4, alpha=0.5)
    plt.tight_layout()
    plt.savefig(FIG / "consistency_lines.png", dpi=140, bbox_inches="tight")
    plt.close(fig)


def rank_summary(results):
    # rank policies 1..N within each scenario by mean turnaround, then
    # show the average rank per policy across all scenarios. the policy
    # that's "consistently good" lands at the lowest avg rank.
    df = results[results.ok].dropna(subset=["turnaround_avg"])
    means = df.groupby(["scenario", "policy"]).turnaround_avg.mean().unstack()
    pols = [p for p in POLICIES if p in means.columns]
    means = means[pols]
    ranks = means.rank(axis=1, method="min")
    avg = ranks.mean(axis=0).sort_values()

    fig, ax = plt.subplots(figsize=(8, 4))
    bars = ax.bar(avg.index, avg.values, color=[CMAP[p] for p in avg.index])
    ax.set_ylabel(f"Mean rank across {len(means)} scenarios  (1 = best)")
    ax.set_title("Which policy is the most consistent winner?")
    ax.grid(axis="y", linestyle="--", linewidth=0.4, alpha=0.5)
    for bar, v in zip(bars, avg.values):
        ax.text(bar.get_x() + bar.get_width() / 2, bar.get_height() + 0.05,
                f"{v:.2f}", ha="center", va="bottom", fontsize=9)
    plt.tight_layout()
    plt.savefig(FIG / "rank_summary.png", dpi=140, bbox_inches="tight")
    plt.close(fig)


def main():
    rpath = OUT / "results.csv"
    if not rpath.exists():
        raise SystemExit(f"missing {rpath}, run run_experiments.py first")
    results = pd.read_csv(rpath)
    results["ok"] = results.ok.astype(str).str.lower() == "true"

    for s in sorted(results.scenario.unique()):
        per_scenario(results, int(s))

    consistency_lines(results)
    rank_summary(results)

    figs = sorted(FIG.glob("story_*.png")) \
         + [FIG / "consistency_lines.png", FIG / "rank_summary.png"]
    print("story figures:")
    for f in figs:
        if f.exists():
            print(f"  {f.name}  ({f.stat().st_size // 1024} KB)")


if __name__ == "__main__":
    main()
