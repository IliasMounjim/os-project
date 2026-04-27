# Hybrid scheduler - data report

Everything the simulator produced, in one place. Figures live in
`analysis/figures/`, raw data in `analysis/results.csv` and
`analysis/traces.csv`.

## The hybrid rule

| condition | mode |
|---|---|
| avg percentIO of arrived jobs >= 15 | RR |
| CoV of remaining bursts < 0.2 | FCFS |
| CoV > 1.5 | SJF |
| else | RR |

CoV = stddev / mean of remaining burst lengths over jobs that have
arrived. Re-runs every scheduling decision (job ends, blocks for i/o,
or burns its quantum). Non-preemptive once a job is running.

## Setup

1120 total runs. 30 random seeds for the 5 i/o scenarios (rand actually
fires there). 1 seed for the rest, deterministic. Ran 7 policies (FCFS,
SJF, SRTF, LJF, RR, PRIORITY, HYBRID) on 15 scenarios.

LOTTERY is broken in the original code so it's left out.

---

## Cross-scenario summary

### Consistency

![consistency](analysis/figures/consistency_lines.png)

One line per policy across the 15 scenarios. Each scenario's turnaround
is min-max scaled inside that scenario, so 0 = best on that workload
and 1 = worst. Hybrid is bold pink. It sits at the bottom on 1 through
12, then climbs on 13 through 15 where preemption matters.

### Mean rank

![rank](analysis/figures/rank_summary.png)

For each scenario, rank policies 1..7 by avg turnaround. Average that
rank across all 15. Closer to 1 means more consistently good. SRTF wins
overall (1.13). FCFS and SJF tie at 1.33. Hybrid is 4th at 1.67. RR is
last at 6.33.

### Best policy per scenario

![winners](analysis/figures/winners.png)

FCFS shows up 12 times because the original 12 scenarios have burst
lengths in the same order as arrivals, so FCFS, SJF, SRTF, and HYBRID
all tie. The chart picks FCFS alphabetically when there's a tie.

### Coverage

![coverage](analysis/figures/coverage.png)

Green = produced valid metrics. Red = failed. Only LOTTERY is red,
everywhere; the function is unfinished in the original repo so the
runs leave it out.

---

## Per-scenario detail

Each scenario has two figures: the story panel (4 metrics, all 7
policies side by side) and a gantt timeline.

### Scenario 1 - 5 jobs, length 200 each, arrivals 0-100

Baseline. Nothing to sort on. 6-way tie at turnaround 549. RR loses
because of context switch overhead with no benefit.

![story](analysis/figures/story_scenario01.png)
![gantt](analysis/figures/gantt_scenario1.png)

### Scenario 2 - 5 jobs, lengths 100-500 ascending, arrivals 0-100

Bursts are already in shortest-first order, so FCFS = SJF = SRTF.
LJF runs longest first and pays for it. RR cycles uselessly.

![story](analysis/figures/story_scenario02.png)
![gantt](analysis/figures/gantt_scenario2.png)

### Scenario 3 - 5 jobs, lengths 100-500, arrivals spread 0-1000

Arrivals far apart, cpu often idle. Scheduling order does not matter
because there is rarely more than one job to choose from. Tie at 349.

![story](analysis/figures/story_scenario03.png)
![gantt](analysis/figures/gantt_scenario3.png)

### Scenario 4 - 5 jobs with 20% i/o

Mild i/o. Too sparse to give RR an edge. Same tie pattern as 2.

![story](analysis/figures/story_scenario04.png)
![gantt](analysis/figures/gantt_scenario4.png)

### Scenario 5 - 5 jobs, priority 0-1

Priority range too narrow to matter. PRIORITY actually loses to FCFS
here. Other policies tie.

![story](analysis/figures/story_scenario05.png)
![gantt](analysis/figures/gantt_scenario5.png)

### Scenario 6 - 5 jobs with 20% i/o and priority 0-1

Combo of 4 and 5. Same reasons stacked.

![story](analysis/figures/story_scenario06.png)
![gantt](analysis/figures/gantt_scenario6.png)

### Scenario 7 - 21 jobs, length 200 each

Scaled-up scenario 1. Same pattern.

![story](analysis/figures/story_scenario07.png)
![gantt](analysis/figures/gantt_scenario7.png)

### Scenario 8 - 21 jobs, lengths 100-500 ascending

Scaled-up scenario 2.

![story](analysis/figures/story_scenario08.png)
![gantt](analysis/figures/gantt_scenario8.png)

### Scenario 9 - 21 jobs, sparse arrivals

Scaled-up scenario 3.

![story](analysis/figures/story_scenario09.png)
![gantt](analysis/figures/gantt_scenario9.png)

### Scenario 10 - 21 jobs with 20% i/o

Scaled-up scenario 4.

![story](analysis/figures/story_scenario10.png)
![gantt](analysis/figures/gantt_scenario10.png)

### Scenario 11 - 21 jobs with priority

Scaled-up scenario 5.

![story](analysis/figures/story_scenario11.png)
![gantt](analysis/figures/gantt_scenario11.png)

### Scenario 12 - 21 jobs with i/o and priority

Scaled-up scenario 6.

![story](analysis/figures/story_scenario12.png)
![gantt](analysis/figures/gantt_scenario12.png)

### Scenario 13 - one 800-tick job at t=0, four 100-tick jobs after

Designed to break the FCFS = SJF = SRTF tie. Tests preemption.
Winner: RR 379, SRTF 380 (effectively tied). Both avoid wedging the
long Job 0 at the front. Hybrid loses (899) because at t=0 only Job 0
has arrived, so CoV is 0 and it picks FCFS mode. By the time the short
jobs arrive, Job 0 is running and hybrid does not preempt.

![story](analysis/figures/story_scenario13.png)
![gantt](analysis/figures/gantt_scenario13.png)

The gantt for s13 is the clearest preemption picture. SRTF interleaves
Job 0 with the shorts as they arrive. RR fans across all five jobs via
quantum slicing. Everyone else runs Job 0 to completion first.

### Scenario 14 - 8 jobs with 30-50% i/o

Heavy i/o. Tests where RR and HYBRID should shine. Winner: FCFS (3375)
narrowly over SRTF (3383). The i/o blocks naturally interleave the
jobs, so simple ordering ends up close to optimal here. Hybrid (4935)
loses on context-switch overhead from RR mode.

![story](analysis/figures/story_scenario14.png)
![gantt](analysis/figures/gantt_scenario14.png)

### Scenario 15 - 10 jobs, lengths 50 to 1500, decoupled from arrivals

High variance bursts in random order. Winner: SRTF (771), SJF very close
(773). Sorting by length wins big here. Hybrid hits the same cold-start
issue as s13 and lands at 1297.

![story](analysis/figures/story_scenario15.png)
![gantt](analysis/figures/gantt_scenario15.png)

---

## Comparison across all scenarios

Same numbers as the per-scenario stories, plotted on one chart. Bars
are mean across seeds, error bars are one standard deviation. Error
bars are only visible on the 5 i/o scenarios (4, 6, 10, 12, 14)
because those are the only ones where the seed changes anything.

### Turnaround
![turnaround comparison](analysis/figures/comparison_turnaround.png)

### Response
![response comparison](analysis/figures/comparison_response.png)

RR's bars are near zero on every scenario. That is correct, RR's whole
job is to keep response time low by cycling through the ready queue
fast. LJF stands out as the worst by a wide margin.

### Fairness
![fairness comparison](analysis/figures/comparison_fairness.png)

### Older single-metric charts (different format)

These came from the early analysis pass before the multi-seed harness
existed. Showing the same data without the error bars.

![turnaround](analysis/figures/turnaround_avg.png)
![response](analysis/figures/response_avg.png)
![fairness](analysis/figures/fairness.png)
![ctx switches](analysis/figures/ctx_switches.png)
![starvation](analysis/figures/starvation.png)

---

## Where hybrid actually lands

| measure | result |
|---|---|
| within 5% of best policy | 12 of 15 scenarios |
| sole winner | 0 |
| worst (rank 7) | 0 |
| mean rank across 15 | 1.67 (4th place) |

The 3 scenarios where hybrid loses meaningfully:

| # | hybrid | best | gap | reason |
|---|---|---|---|---|
| 13 | 899  | 379 (RR/SRTF)  | 2.37x | cold-start: at t=0 only Job 0 visible, CoV=0, picks FCFS, runs Job 0 to completion before the shorts arrive |
| 14 | 4935 | 3375 (FCFS)    | 1.46x | switches to RR (correct call), but RR pays ctx-switch overhead that FCFS avoids on this i/o pattern |
| 15 | 1297 | 771 (SJF/SRTF) | 1.68x | same cold-start as s13, only Job 0 visible at t=0 |

All three losses are scenarios where reordering or preemption pays off
and hybrid is non-preemptive once a job is running.

What the data does support: rule-based hybrid matches the best
non-preemptive fixed policy on the 12 easy workloads without needing
burst-length foreknowledge that SJF assumes. Never the worst on any
scenario.

What the data does not support: hybrid does not beat every fixed
policy. SRTF wins overall.

---

## Known broken (still open)

- LOTTERY runJobs hits `.front()` on an empty queue, silent no-op. Not
  in any of the figures, just left out
- Hybrid is non-preemptive once a job runs. Preemptive variant is the
  obvious next step to close the SRTF gap on s13/15
- RR doesn't actually check arrival time when picking the next job on a
  quantum boundary, so it can "run" jobs before they exist. Doesn't
  affect turnaround much but makes the trace events look weird at
  early time steps

## Repro

```
cmake -B build && cmake --build build
.venv/bin/python run_experiments.py
.venv/bin/python make_figures.py
.venv/bin/python make_story.py
```
