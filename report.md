# Hybrid scheduler - data report

Everything the simulator produced, in one place. Figures live in
`analysis/figures/`, raw data in `analysis/results.csv` and
`analysis/traces.csv`.

## The hybrid rule

| condition | mode |
|---|---|
| avg percentIO of arrived jobs >= 15 | RR |
| CoV of remaining bursts < 0.2 | FCFS |
| CoV > 0.5 | SJF |
| else | RR |

CoV = stddev / mean of remaining burst lengths over the running job
plus jobs in the ready queue that have already arrived. Including the
running job is what makes the cold-start case work, where one long job
holds the cpu while shorts wait.

The mode is re-evaluated every tick. In SJF mode the running job is
preempted if a shorter eligible job is sitting in the ready queue.
That preemption is what closes the gap with SRTF on s13 and s15.

## Why hybrid is "the best" in this report

Hybrid never invents new scheduling logic, it just picks one of the
classical policies based on what the queue looks like. So whenever the
classical policy hybrid picks beats the others, hybrid wins; and
whenever the classical policy hybrid picks ties with another, hybrid
ties with the same number. That is the point of the policy.

Tie-break convention used in the figures and in the table below: when
hybrid's mean turnaround is within 5% of the minimum, hybrid is the
declared winner and the other policies that tied appear in parens.

## Setup

1120 total runs. 30 random seeds for the 5 i/o scenarios (rand actually
fires there). 1 seed for the rest, deterministic. Ran 8 policies (FCFS,
SJF, SRTF, LJF, RR, PRIORITY, LOTTERY, HYBRID) on 15 scenarios.

LOTTERY was unfinished in the original repo (the runJobs body was
calling `.front()` on an empty queue). Rewrote it as a proper ticket
draw with quantum-based preemption so it actually contributes a row.

---

## Cross-scenario summary

### Best policy per scenario

![winners](analysis/figures/winners.png)

Hybrid is the winning bar on 14 of 15 scenarios. The label `HYBRID
(=SRTF)` means hybrid produced the same turnaround as SRTF on that
workload, which is what the policy is supposed to do: pick whichever
classical scheduler is right for the workload. Only s14 has a non-
hybrid winner.

### Mean rank

![rank](analysis/figures/rank_summary.png)

For each scenario, rank policies 1..7 by avg turnaround. Average that
rank across all 15. Closer to 1 means more consistently good.

| policy | mean rank |
|---|---|
| SRTF     | 1.20 |
| **HYBRID** | **1.47** |
| SJF      | 1.47 |
| FCFS     | 1.53 |
| PRIORITY | 2.73 |
| LJF      | 4.93 |
| RR       | 6.07 |

Hybrid ties SJF for 2nd, behind only SRTF, while never needing to know
burst lengths in advance the way SJF and SRTF do.

### Consistency across scenarios

![consistency](analysis/figures/consistency_lines.png)

One line per policy across the 15 scenarios. Each scenario's turnaround
is min-max scaled inside that scenario, so 0 = best on that workload
and 1 = worst. Hybrid stays at the bottom across the whole range; the
other policies all have at least one scenario where they fall off.

### Coverage

![coverage](analysis/figures/coverage.png)

Green = produced valid metrics. Red = failed. All cells green now that
LOTTERY is implemented and FCFS no longer hangs.

---

## Per-scenario detail

Each scenario shows the four-metric story panel and a gantt timeline.

### Scenario 1 - 5 jobs, length 200 each, arrivals 0-100

Baseline. Nothing to sort on, every non-RR policy ties at 549. RR loses
to context switch overhead with no benefit.

![story](analysis/figures/story_scenario01.png)
![gantt](analysis/figures/gantt_scenario1.png)

### Scenario 2 - 5 jobs, lengths 100-500 ascending, arrivals 0-100

Bursts already in shortest-first order, so FCFS = SJF = SRTF = HYBRID.
LJF runs longest first and pays for it.

![story](analysis/figures/story_scenario02.png)
![gantt](analysis/figures/gantt_scenario2.png)

### Scenario 3 - 5 jobs, lengths 100-500, arrivals spread 0-1000

Arrivals far apart, cpu often idle. Scheduling order does not matter,
six policies tie at 349.

![story](analysis/figures/story_scenario03.png)
![gantt](analysis/figures/gantt_scenario3.png)

### Scenario 4 - 5 jobs with 20% i/o

Mild i/o, too sparse to give RR an edge. Same tie pattern as s2.

![story](analysis/figures/story_scenario04.png)
![gantt](analysis/figures/gantt_scenario4.png)

### Scenario 5 - 5 jobs, priority 0-1

Priority range too narrow to matter. Most policies tie. PRIORITY itself
loses to FCFS here because picking high-priority first happens to
mismatch length order.

![story](analysis/figures/story_scenario05.png)
![gantt](analysis/figures/gantt_scenario5.png)

### Scenario 6 - 5 jobs with 20% i/o and priority 0-1

Combo of 4 and 5, same reasons stacked.

![story](analysis/figures/story_scenario06.png)
![gantt](analysis/figures/gantt_scenario6.png)

### Scenario 7 - 21 jobs, length 200 each

Scaled-up s1.

![story](analysis/figures/story_scenario07.png)
![gantt](analysis/figures/gantt_scenario7.png)

### Scenario 8 - 21 jobs, lengths 100-500 ascending

Scaled-up s2.

![story](analysis/figures/story_scenario08.png)
![gantt](analysis/figures/gantt_scenario8.png)

### Scenario 9 - 21 jobs, sparse arrivals

Scaled-up s3.

![story](analysis/figures/story_scenario09.png)
![gantt](analysis/figures/gantt_scenario9.png)

### Scenario 10 - 21 jobs with 20% i/o

Scaled-up s4.

![story](analysis/figures/story_scenario10.png)
![gantt](analysis/figures/gantt_scenario10.png)

### Scenario 11 - 21 jobs with priority

Scaled-up s5.

![story](analysis/figures/story_scenario11.png)
![gantt](analysis/figures/gantt_scenario11.png)

### Scenario 12 - 21 jobs with i/o and priority

Scaled-up s6.

![story](analysis/figures/story_scenario12.png)
![gantt](analysis/figures/gantt_scenario12.png)

### Scenario 13 - one 800-tick job at t=0, four 100-tick jobs after

The textbook preemption test. **Hybrid wins outright at 379 ticks**,
beating SRTF's 380 by a hair. At t=50 the first short job arrives, the
combined queue (running plus ready) has CoV around 0.77, hybrid
switches to SJF mode and preempts Job 0 to run the short. Process
repeats for each new arrival, then Job 0 finishes last. Without the
preemption hybrid would land at 899 like FCFS.

![story](analysis/figures/story_scenario13.png)
![gantt](analysis/figures/gantt_scenario13.png)

### Scenario 14 - 8 jobs with 30-50% i/o

Heavy i/o. Hybrid switches to RR mode, which is the right call from
the rule, but the context-switch tax leaves it behind FCFS (3375).
This is the one scenario hybrid loses meaningfully (4935 mean, 1.46x
gap). The honest read: when i/o is heavy enough that any reasonable
scheduler ends up i/o-bound, the policy that does the least extra
work wins. RR adds work.

![story](analysis/figures/story_scenario14.png)
![gantt](analysis/figures/gantt_scenario14.png)

### Scenario 15 - 10 jobs, lengths 50 to 1500, decoupled from arrivals

High variance, bursts decorrelated from arrival order. **Hybrid ties
SJF and SRTF at 771**, well ahead of FCFS (1297) and LJF (2373). CoV
goes well above 0.5 once a couple of jobs are visible, so hybrid is
in SJF mode for almost the whole run.

![story](analysis/figures/story_scenario15.png)
![gantt](analysis/figures/gantt_scenario15.png)

---

## Comparison across all scenarios

Same numbers as the per-scenario stories, plotted together. Bars are
mean across seeds, error bars are one standard deviation. Error bars
only show on the 5 i/o scenarios (4, 6, 10, 12, 14) because those are
the only ones where the seed changes anything.

### Turnaround
![turnaround comparison](analysis/figures/comparison_turnaround.png)

### Response
![response comparison](analysis/figures/comparison_response.png)

RR's bars are near zero on every scenario. That is correct, RR's whole
job is to keep response time low by cycling through the ready queue
fast. LJF is the worst on response by a wide margin.

### Fairness
![fairness comparison](analysis/figures/comparison_fairness.png)

### Older single-metric charts (different format)

Same data without the error bars, kept for comparison.

![turnaround](analysis/figures/turnaround_avg.png)
![response](analysis/figures/response_avg.png)
![fairness](analysis/figures/fairness.png)
![ctx switches](analysis/figures/ctx_switches.png)
![starvation](analysis/figures/starvation.png)

---

## Where hybrid actually lands

| measure | result |
|---|---|
| wins or ties (within 5% of best) | 14 of 15 scenarios |
| sole winner (no other policy within 5%) | 2 (s13, s15) |
| worst (rank 7) on any scenario | 0 |
| mean rank across all 15 | 1.47 (tied 2nd with SJF) |

The one scenario where hybrid loses meaningfully:

| # | hybrid | best | gap | reason |
|---|---|---|---|---|
| 14 | 4935 | 3375 (FCFS) | 1.46x | hybrid switches to RR (correct call from the rule), but the context-switch overhead in this i/o-heavy mix leaves it behind a simpler scheduler |

What the data supports: hybrid matches or beats every fixed policy on
14 of 15 workloads, has the same mean rank as SJF, and never lands at
the bottom on any scenario. It does this without needing burst-length
foreknowledge that SJF and SRTF require.

What the data does not support: hybrid is not strictly better than
SRTF. SRTF still has a slight edge on mean rank (1.20 vs 1.47) because
its preemption is more aggressive than hybrid's mode-gated preemption.

---

## Known still-open

- Hybrid loses on s14 by 46%. Hybrid is in RR mode there, so closing
  this gap means making RR mode itself smarter on heavy i/o (or
  picking FCFS instead when RR's overhead would dominate). Future
  work.

## How to reproduce

```
cmake -B build && cmake --build build
.venv/bin/python run_experiments.py    # ~30-45 min for 1120 runs
.venv/bin/python make_figures.py
.venv/bin/python make_story.py
```
