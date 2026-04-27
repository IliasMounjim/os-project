#!/usr/bin/env python3
# reads analysis/results.csv + analysis/traces.csv and produces all the
# figures we use for the slides. comparison charts show mean and a one-
# sigma error bar across seeds, gantt charts pick seed 1 since the trace
# would otherwise be repeated 30 times.

from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.patches as mpatches
import matplotlib.pyplot as plt
import pandas as pd

ROOT = Path(__file__).parent
OUT = ROOT / "analysis"
FIG = OUT / "figures"
FIG.mkdir(parents=True, exist_ok=True)

POLICIES = ["FCFS", "SJF", "SRTF", "LJF", "RR", "PRIORITY", "HYBRID"]
COLORS = plt.rcParams["axes.prop_cycle"].by_key()["color"]


def job_color(job_id):
    if job_id < 0:
        return "#cccccc"  # idle gap
    return COLORS[job_id % len(COLORS)]


def gantt(traces, scenario):
    # use seed 1 for the gantt picture, the rest would just repeat
    sub = traces[(traces.scenario == scenario) & (traces.seed == 1)]
    if sub.empty:
        return
    policies_present = [p for p in POLICIES if p in sub.policy.values]
    if not policies_present:
        return

    n = len(policies_present)
    fig, axes = plt.subplots(n, 1, figsize=(14, max(3, n * 1.8)),
                             sharex=True, squeeze=False)
    fig.suptitle(f"Gantt - Scenario {scenario}", fontsize=12)

    for ax, policy in zip(axes[:, 0], policies_present):
        rows = sub[sub.policy == policy]
        for _, r in rows.iterrows():
            jid = int(r.job_id)
            start = float(r.start)
            width = float(r.end) - start
            ypos = jid if jid >= 0 else -1
            ax.broken_barh([(start, width)], (ypos - 0.4, 0.8),
                           facecolors=job_color(jid),
                           edgecolors="white", linewidth=0.5)
        real = sorted({j for j in rows.job_id if j >= 0})
        ax.set_yticks(real)
        ax.set_yticklabels([f"J{j}" for j in real], fontsize=7)
        ax.set_ylabel(policy, fontsize=8)
        ax.grid(axis="x", linestyle="--", linewidth=0.4, alpha=0.5)

    axes[-1, 0].set_xlabel("Time")
    all_jobs = sorted(sub.job_id.unique())
    patches = [mpatches.Patch(color=job_color(j),
                              label=f"Job {j}" if j >= 0 else "Idle")
               for j in all_jobs]
    fig.legend(handles=patches, loc="lower right",
               ncol=min(8, len(patches)), fontsize=7, framealpha=0.8)
    plt.tight_layout(rect=[0, 0.04, 1, 0.97])
    plt.savefig(FIG / f"gantt_scenario{scenario}.png", dpi=140,
                bbox_inches="tight")
    plt.close(fig)


def comparison(results, metric, title, ylabel, fname):
    # mean + std across seeds, plotted as grouped bars with error bars
    df = results[results.ok].dropna(subset=[metric])
    if df.empty:
        return
    agg = df.groupby(["scenario", "policy"])[metric].agg(["mean", "std"]).reset_index()
    agg["std"] = agg["std"].fillna(0)

    pols = [p for p in POLICIES if p in agg.policy.values]
    scens = sorted(agg.scenario.unique())
    width = 0.8 / max(len(pols), 1)

    fig, ax = plt.subplots(figsize=(max(10, len(scens) * 1.2), 5))
    for i, pol in enumerate(pols):
        col = agg[agg.policy == pol].set_index("scenario").reindex(scens)
        offsets = [s + (i - len(pols) / 2 + 0.5) * width
                   for s in range(len(scens))]
        ax.bar(offsets, col["mean"].values, width=width * 0.9,
               yerr=col["std"].values, label=pol,
               color=COLORS[i % len(COLORS)],
               error_kw=dict(elinewidth=0.6, capsize=2, alpha=0.7))
    ax.set_xticks(range(len(scens)))
    ax.set_xticklabels([f"S{s}" for s in scens])
    ax.set_xlabel("Scenario")
    ax.set_ylabel(ylabel)
    ax.set_title(title)
    ax.legend(loc="upper left", bbox_to_anchor=(1.0, 1.0), fontsize=8)
    plt.tight_layout()
    plt.savefig(FIG / fname, dpi=140, bbox_inches="tight")
    plt.close(fig)


def coverage(results):
    pols = [p for p in POLICIES if p in results.policy.values]
    scens = sorted(results.scenario.unique())
    # cell is 1 if at least one seed succeeded
    pv = (results.assign(ok=results.ok.astype(int))
                 .groupby(["policy", "scenario"]).ok.max()
                 .unstack().reindex(index=pols, columns=scens).fillna(0))

    fig, ax = plt.subplots(figsize=(max(8, len(scens) * 0.7),
                                    max(3, len(pols) * 0.55)))
    ax.imshow(pv.values, aspect="auto", cmap="RdYlGn", vmin=0, vmax=1)
    ax.set_xticks(range(len(scens)))
    ax.set_xticklabels([f"S{s}" for s in scens], fontsize=8)
    ax.set_yticks(range(len(pols)))
    ax.set_yticklabels(pols, fontsize=8)
    ax.set_title("Coverage: policy x scenario  (green = ran, red = failed)")
    for i in range(len(pols)):
        for j in range(len(scens)):
            v = int(pv.values[i, j])
            ax.text(j, i, "OK" if v else "X", ha="center", va="center",
                    color="white" if v else "black",
                    fontsize=7, fontweight="bold")
    plt.tight_layout()
    plt.savefig(FIG / "coverage.png", dpi=140, bbox_inches="tight")
    plt.close(fig)


def winners(results):
    df = results[results.ok].dropna(subset=["turnaround_avg"])
    if df.empty:
        return
    means = df.groupby(["scenario", "policy"]).turnaround_avg.mean().reset_index()

    # for each scenario, build the winner label. if hybrid is within 5%
    # of the best, hybrid wins; the tied policies show up in parentheses
    rows = []
    for sc, group in means.groupby("scenario"):
        best = group.turnaround_avg.min()
        close = group[group.turnaround_avg <= best * 1.05]
        close_pols = list(close.policy)
        if "HYBRID" in close_pols:
            others = sorted(p for p in close_pols if p != "HYBRID")
            label = "HYBRID" if not others else f"HYBRID (={','.join(others)})"
            color_pol = "HYBRID"
            val = float(close[close.policy == "HYBRID"].turnaround_avg.iloc[0])
        else:
            color_pol = close.iloc[0].policy
            label = color_pol
            val = float(close.iloc[0].turnaround_avg)
        rows.append({"scenario": sc, "label": label, "value": val,
                     "color_pol": color_pol})

    rows.sort(key=lambda r: r["scenario"])
    scens  = [r["scenario"] for r in rows]
    labels = [r["label"]    for r in rows]
    vals   = [r["value"]    for r in rows]
    cols   = [COLORS[POLICIES.index(r["color_pol"]) % len(COLORS)]
              if r["color_pol"] in POLICIES else "#888" for r in rows]

    fig, ax = plt.subplots(figsize=(9, max(3, len(scens) * 0.45)))
    bars = ax.barh([f"Scenario {s}" for s in scens], vals, color=cols, height=0.6)
    ax.set_xlabel("Avg Turnaround Time (lower is better)")
    ax.set_title("Best policy per scenario (hybrid wins ties)")
    for b, lbl, v in zip(bars, labels, vals):
        ax.text(b.get_width() * 0.01, b.get_y() + b.get_height() / 2,
                f"{lbl}  ({v:.0f})", va="center", ha="left", fontsize=8)
    seen = {}
    for r, c in zip(rows, cols):
        seen[r["color_pol"]] = c
    ax.legend(handles=[mpatches.Patch(color=c, label=p) for p, c in seen.items()],
              loc="lower right", fontsize=8)
    plt.tight_layout()
    plt.savefig(FIG / "winners.png", dpi=140, bbox_inches="tight")
    plt.close(fig)


def main():
    rpath = OUT / "results.csv"
    if not rpath.exists():
        raise SystemExit(f"missing {rpath}, run run_experiments.py first")

    results = pd.read_csv(rpath)
    results["ok"] = results.ok.astype(str).str.lower() == "true"

    tpath = OUT / "traces.csv"
    if tpath.exists():
        traces = pd.read_csv(tpath)
        for s in sorted(traces.scenario.unique()):
            gantt(traces, int(s))

    comparison(results, "turnaround_avg",
               "Avg Turnaround by Policy and Scenario (mean +/- std)",
               "Turnaround", "comparison_turnaround.png")
    comparison(results, "response_avg",
               "Avg Response by Policy and Scenario (mean +/- std)",
               "Response", "comparison_response.png")
    comparison(results, "fairness",
               "Fairness by Policy and Scenario",
               "Fairness", "comparison_fairness.png")

    coverage(results)
    winners(results)

    figs = sorted(FIG.glob("*.png"))
    print(f"figures in {FIG}:")
    for f in figs:
        print(f"  {f.name}  ({f.stat().st_size // 1024} KB)")


if __name__ == "__main__":
    main()
