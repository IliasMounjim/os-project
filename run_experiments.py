#!/usr/bin/env python3
# runs schedulerSim across every (policy, scenario, seed) and dumps two
# csvs into analysis/. results.csv has one row per run, traces.csv has
# one row per execution event for the gantt charts.

import csv
import json
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).parent
BIN = ROOT / "build" / "bin" / "schedulerSim"
JSON_DIR = ROOT / "json"
OUT = ROOT / "analysis"
OUT.mkdir(exist_ok=True)

POLICIES = ["FCFS", "SJF", "SRTF", "LJF", "RR", "PRIORITY", "HYBRID"]
SCENARIOS = list(range(1, 16))
# how many seeds per (policy, scenario). only matters for io-heavy
# scenarios since the rest are deterministic anyway.
SEEDS = list(range(1, 31))

EVENT_RX = re.compile(r"\(id - (-?\d+)\s*,\s*start - (-?\d+),\s*end - (-?\d+)\)")
METRIC_RX = re.compile(r"^METRICS,([^,]+),(.+)$", re.MULTILINE)

RESULTS_COLS = ["policy", "scenario", "seed", "ok",
                "turnaround_avg", "turnaround_max",
                "response_avg", "response_max",
                "fairness", "starvation", "ctx_switches", "jobs"]
TRACE_COLS = ["policy", "scenario", "seed", "job_id", "start", "end"]


def run_one(policy, scenario, seed):
    # always use --csv so we get one METRICS line + the (id-x,start-y,end-z) events
    try:
        p = subprocess.run(
            [str(BIN), "-p", policy, "-j",
             str(JSON_DIR / f"scenario{scenario}.json"),
             "-s", str(seed), "--csv"],
            capture_output=True, text=True, timeout=20,
        )
    except subprocess.TimeoutExpired:
        return None, []

    out = p.stdout
    row = {"policy": policy, "scenario": scenario, "seed": seed, "ok": False}

    m = METRIC_RX.search(out)
    if m:
        vals = m.group(2).split(",")
        if len(vals) == 8:
            keys = ["turnaround_avg", "turnaround_max", "response_avg",
                    "response_max", "fairness", "starvation",
                    "ctx_switches", "jobs"]
            for k, v in zip(keys, vals):
                row[k] = float(v)
            row["ok"] = True

    events = [{"policy": policy, "scenario": scenario, "seed": seed,
               "job_id": int(g[0]), "start": int(g[1]), "end": int(g[2])}
              for g in EVENT_RX.findall(out)]
    return row, events


def main():
    if not BIN.exists():
        sys.exit(f"binary missing: {BIN}  (run cmake --build build first)")

    results = []
    traces = []
    total = len(POLICIES) * len(SCENARIOS) * len(SEEDS)
    done = 0
    for policy in POLICIES:
        for scenario in SCENARIOS:
            # io scenarios deserve all seeds; non-io is deterministic
            # so one seed is enough
            with open(JSON_DIR / f"scenario{scenario}.json") as f:
                jobs = json.load(f)
            has_io = any(float(j.get("percentIO", 0)) > 0 for j in jobs.values())
            seeds_for_combo = SEEDS if has_io else SEEDS[:1]
            for seed in seeds_for_combo:
                done += 1
                pct = int(done * 100 / total)
                print(f"  [{pct:3d}%] {policy:9s} s{scenario:2d} seed{seed:3d}",
                      end="\r", flush=True)
                row, evs = run_one(policy, scenario, seed)
                if row is None:
                    continue
                results.append(row)
                traces.extend(evs)
    print()

    rpath = OUT / "results.csv"
    with open(rpath, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=RESULTS_COLS)
        w.writeheader()
        for r in results:
            for k in RESULTS_COLS:
                r.setdefault(k, None)
            w.writerow(r)

    tpath = OUT / "traces.csv"
    with open(tpath, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=TRACE_COLS)
        w.writeheader()
        w.writerows(traces)

    ok = sum(1 for r in results if r["ok"])
    print(f"results -> {rpath}  ({ok}/{len(results)} runs ok)")
    print(f"traces  -> {tpath}  ({len(traces)} events)")
    fails = sorted(set((r["policy"], r["scenario"]) for r in results if not r["ok"]))
    if fails:
        print("failed combos:")
        for p, s in fails:
            print(f"  {p} scenario{s}")


if __name__ == "__main__":
    main()
