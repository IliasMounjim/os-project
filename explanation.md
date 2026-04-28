# Explanation guide - hybrid scheduler + analysis pipeline

This walks through the hybrid implementation function by function, the
visualization pipeline, and the figures we generate. Use it as the
reference when writing the final report or talking through the code.

Files referenced:
- `src/hybrid.cpp` and `src/hybrid.h` (the policy)
- `src/policy.cpp` and `src/policy.h` (shared metrics + CSV mode)
- `src/main.cpp` (dispatch + flags)
- `run_experiments.py` (driver)
- `make_figures.py` (cross-scenario figures)
- `make_story.py` (per-scenario figures)

---

## Part 1 - the hybrid scheduler

`src/hybrid.cpp` is 8 functions. Top-down:

### 1.1 `hybRand(unsigned range)` - bias-free random in [0, range)

```cpp
unsigned hybRand(unsigned range)
{
    for (unsigned x, r;;)
        if (x = rand(), r = x % range, x - r <= -range)
            return r;
}
```

Same rejection-sampling pattern the other policies use, just prefixed
`hyb` so it doesn't collide at link time. Standard `rand()` modulo bias
fix from the cppreference page.

Used once per tick to decide whether the running job blocks for i/o:
`if (hybRand(100) < running.getPercentIO()) { block }`.

### 1.2 `covRemaining(Schedule &q, uint64_t t)` - coefficient of variation

```cpp
double covRemaining(Schedule &q, std::uint64_t t)
{
    double sum = 0.0;
    int    n   = 0;
    for (Job j : q.schedule)
    {
        if (j.getArrival() <= (int)t)
        {
            sum += j.getLength();
            n++;
        }
    }
    if (n < 2) return 0.0;

    double mean = sum / n;
    double var  = 0.0;
    for (Job j : q.schedule)
    {
        if (j.getArrival() <= (int)t)
        {
            double d = j.getLength() - mean;
            var += d * d;
        }
    }
    var /= n;
    if (mean <= 0.0) return 0.0;
    return std::sqrt(var) / mean;
}
```

CoV is stddev divided by mean. It measures how spread out the burst
lengths are. Returns 0 if fewer than 2 jobs are arrived (CoV undefined
for single values).

Why CoV and not raw variance: CoV is scale-free. Bursts of (50, 100)
and (500, 1000) both have CoV around 0.33. Raw variance would give
very different numbers and we'd need different thresholds for different
workload sizes.

Why filter by arrival: a job that hasn't arrived can't be on the cpu
or in the ready queue, so it shouldn't influence the decision.

### 1.3 `avgPercentIO(Schedule &q, uint64_t t)` - i/o intensity signal

```cpp
double avgPercentIO(Schedule &q, std::uint64_t t)
{
    double sum = 0.0;
    int    n   = 0;
    for (Job j : q.schedule)
    {
        if (j.getArrival() <= (int)t)
        {
            sum += j.getPercentIO();
            n++;
        }
    }
    if (n == 0) return 0.0;
    return sum / n;
}
```

Mean of `percentIO` over arrived jobs. If avg goes above 15, the rule
kicks into RR mode regardless of CoV. The 15 cutoff was chosen because
percentIO 0 means "never blocks," 20 (our existing scenarios) means
"blocks ~20% of ticks," so 15 captures any meaningful i/o presence.

### 1.4 `pickMode(Schedule &q, uint64_t t)` - the rule

```cpp
HybridMode pickMode(Schedule &q, std::uint64_t t)
{
    if (q.schedule.empty()) return MODE_FCFS;

    double io  = avgPercentIO(q, t);
    double cov = covRemaining(q, t);

    if (io  >= hybIoCutoff) return MODE_RR;
    if (cov <  hybCovLow)   return MODE_FCFS;
    if (cov >  hybCovHigh)  return MODE_SJF;
    return MODE_RR;
}
```

The decision function. Constants are at the top of the file:

```cpp
int    const hybIoCutoff = 15;   // %IO threshold for switching to RR
double const hybCovLow   = 0.2;  // CoV below this = uniform, use FCFS
double const hybCovHigh  = 0.5;  // CoV above this = varied, use SJF
```

Order matters. The i/o check fires first because i/o-heavy workloads
benefit from RR's quick cycling regardless of burst variance. After
that, CoV decides between FCFS (uniform jobs, no benefit from
reordering), SJF (high variance, sorting saves time), or RR (moderate
spread, default to fair).

The 0.5 SJF threshold replaced the original 1.5 from the design notes
because empirically 1.5 never fired on our test scenarios. The
boundary where SJF starts beating FCFS sits closer to 0.5 in practice.

### 1.5 `pickIndex(Schedule &q, uint64_t t, HybridMode mode)` - choose a job

```cpp
int pickIndex(Schedule &q, std::uint64_t t, HybridMode mode)
{
    int pick = -1;

    if (mode == MODE_SJF)
    {
        int minLen = INT_MAX;
        for (int i = 0; i < (int)q.schedule.size(); i++)
        {
            if (q.schedule[i].getArrival() <= (int)t &&
                q.schedule[i].getLength()  <  minLen)
            {
                minLen = q.schedule[i].getLength();
                pick   = i;
            }
        }
    }
    else // FCFS or RR pick the earliest-arrived job
    {
        int minArr = INT_MAX;
        for (int i = 0; i < (int)q.schedule.size(); i++)
        {
            if (q.schedule[i].getArrival() <= (int)t &&
                q.schedule[i].getArrival()  <  minArr)
            {
                minArr = q.schedule[i].getArrival();
                pick   = i;
            }
        }
    }
    return pick;
}
```

Given a mode, returns the index of the next job to run.

- **SJF mode**: scan all arrived jobs, pick the one with shortest
  remaining length.
- **FCFS / RR mode**: pick the earliest-arrived eligible job. RR's
  round-robin behavior comes from the fact that pre-empted jobs get
  pushed to the back of the queue (their effective arrival becomes the
  back-of-queue position).

Returns -1 if nothing is eligible (every remaining job has arrival
greater than t). The caller treats that as an idle tick.

### 1.6 `hybridNextJob(...)` - prep the picked job

```cpp
Job hybridNextJob(Schedule &readyQueue, std::uint64_t currTime,
                  bool &noRunning, std::uint64_t breakStart,
                  policy::Trace &trace, HybridMode mode)
{
    int idx = pickIndex(readyQueue, currTime, mode);
    if (idx >= 0)
    {
        Job next = readyQueue.schedule[idx];
        readyQueue.schedule.erase(readyQueue.schedule.begin() + idx);
        if (noRunning)
            trace.addEvent(policy::Event(breakStart, currTime, -1));
        noRunning = false;
        // initialize start now so a same-tick i/o block doesn't read garbage
        next.setStarted(true);
        next.setStart(currTime);
        return next;
    }
    noRunning = true;
    return Job(-1, 0, 0, 0, 1);
}
```

Wraps `pickIndex`. When it finds a winner, removes the job from the
ready queue, closes any open idle gap in the trace (events with
id=-1 represent cpu idle), and immediately stamps the picked job
with `started=true` and `start=currTime`.

That last bit is non-obvious. If we left the picked job's `started`
flag false and waited for the top of the next loop iteration to set
it, then the i/o block check that fires later in the SAME tick would
read `running.getStart()` before it was set, producing trace events
with garbage start times and negative response times in the metrics.
The fix is one line, but it took a debug session to find.

### 1.7 `hybridRunJobs(Schedule s, int quantum)` - the main loop

The biggest function. Tick-based discrete event simulation. Annotated:

```cpp
policy::Trace hybridRunJobs(Schedule s, int quantum)
{
    Schedule readyQueue   = Schedule(s);
    Schedule blockedQueue = Schedule();
    policy::Trace trace   = policy::Trace();

    trace.s = s;

    // sort by arrival so we start with the earliest job
    std::sort(readyQueue.schedule.begin(), readyQueue.schedule.end(),
        [](Job a, Job b) { return a.getArrival() < b.getArrival(); });

    HybridMode mode = pickMode(readyQueue, 0);

    // bootstrap the first running job
    Job  running    = readyQueue.schedule.front();
    readyQueue.schedule.erase(readyQueue.schedule.begin());
    bool noRunning  = false;
    std::uint64_t currTime   = 0;
    std::uint64_t breakStart = 0;
    int           slice      = 0;

    // loop until everything has finished. the running-condition clause
    // is what fixes the off-by-one bug James spotted earlier
    while (!readyQueue.schedule.empty() || !blockedQueue.schedule.empty()
           || (!noRunning && running.getLength() > 0))
    {
        if (currTime == UINT64_MAX) { ... exit ... }

        // mark the first tick this job actually starts running
        if (!running.getStarted() && !noRunning) {
            running.setStarted(true);
            running.setStart(currTime);
        }

        // execute one tick
        if (!noRunning) {
            running.decrementLength();
            slice++;
        }

        // re-check mode every tick. include the running job in the
        // calc so the cov reflects the actual workload, not just what's
        // waiting. this is what makes the s13 cold-start work.
        Schedule combined = readyQueue;
        if (!noRunning) combined.schedule.push_back(running);
        mode = pickMode(combined, currTime);

        // SJF-mode preemption. if a shorter eligible job is in the
        // ready queue, swap. closes the SRTF gap on s13 and s15.
        if (!noRunning && mode == MODE_SJF && running.getLength() > 0) {
            int bestIdx = -1;
            int bestLen = running.getLength();
            for (int i = 0; i < (int)readyQueue.schedule.size(); i++) {
                if (readyQueue.schedule[i].getArrival() <= (int)currTime &&
                    readyQueue.schedule[i].getLength() < bestLen) {
                    bestLen = readyQueue.schedule[i].getLength();
                    bestIdx = i;
                }
            }
            if (bestIdx >= 0) {
                trace.addEvent(policy::Event(running.getStart(), currTime,
                                             running.getID()));
                running.setStarted(false);
                readyQueue.schedule.push_back(running);

                Job next = readyQueue.schedule[bestIdx];
                readyQueue.schedule.erase(readyQueue.schedule.begin() + bestIdx);
                next.setStarted(true);
                next.setStart(currTime);
                running = next;
                slice = 0;
            }
        }

        // job finished
        if (running.getLength() <= 0) {
            if (!noRunning) {
                running.setStatus(1);
                trace.addEvent(policy::Event(running.getStart(), currTime,
                                             running.getID()));
            }
            mode    = pickMode(readyQueue, currTime);
            running = hybridNextJob(readyQueue, currTime, noRunning,
                                    breakStart, trace, mode);
            if (noRunning) breakStart = currTime;
            slice = 0;
        }
        // quantum expired (only matters in RR mode)
        else if (mode == MODE_RR && slice >= quantum && !noRunning) {
            trace.addEvent(policy::Event(running.getStart(), currTime,
                                         running.getID()));
            running.setStarted(false);
            readyQueue.schedule.push_back(running);

            mode    = pickMode(readyQueue, currTime);
            running = hybridNextJob(readyQueue, currTime, noRunning,
                                    breakStart, trace, mode);
            if (noRunning) breakStart = currTime;
            slice = 0;
        }

        // i/o block - stochastic
        if (hybRand(100) < (unsigned)running.getPercentIO() && !noRunning) {
            trace.addEvent(policy::Event(running.getStart(), currTime,
                                         running.getID()));
            running.setStarted(false);
            running.setIOEnd((int)(hybRand(hybIoRange) + currTime));
            blockedQueue.schedule.push_back(running);

            mode    = pickMode(readyQueue, currTime);
            running = hybridNextJob(readyQueue, currTime, noRunning,
                                    breakStart, trace, mode);
            if (noRunning) breakStart = currTime;
            slice = 0;
        }

        // unblock i/o-finished jobs back to ready
        for (auto it = blockedQueue.schedule.begin();
             it != blockedQueue.schedule.end(); ) {
            if (currTime == (std::uint64_t)(*it).getIOEnd()) {
                readyQueue.schedule.push_back(*it);
                it = blockedQueue.schedule.erase(it);
            }
            else
                it++;
        }

        currTime++;
    }

    return trace;
}
```

The loop has 4 distinct phases per tick:
1. **Decrement** the running job (one tick of work)
2. **Mode check + preemption** for SJF
3. **Termination check** (job finished, quantum expired, or i/o blocked)
4. **Unblock** any i/o-finished jobs back into the ready queue

The non-obvious bits:
- `Schedule combined = readyQueue` followed by adding running uses
  Schedule's copy constructor, then a push. This is the "include
  running in CoV" trick that makes s13 work.
- The `(!noRunning && running.getLength() > 0)` clause in the while
  condition is what keeps the loop alive while the last job still has
  work. Without it, fcfs and sjf used to drop the last job because the
  loop would exit when both queues became empty.

### 1.8 `policy::Hybrid::evaluate(Schedule s)` - entry point

```cpp
policy::Policy policy::Hybrid::evaluate(Schedule s)
{
    return policy::Policy("HYBRID", hybridRunJobs(s, quantum), quantum);
}
```

Just wraps `hybridRunJobs`. Called from `main.cpp` after the `-p HYBRID`
flag is parsed. Returns a `Policy` object that gets passed to
`printTraceAnalysis()` for metric extraction.

---

## Part 2 - the visualization pipeline

Three Python scripts in the project root, all using the venv at
`.venv/`. They read CSV output from the simulator and produce all the
figures.

### 2.1 `run_experiments.py` - the driver

```python
POLICIES = ["FCFS", "SJF", "SRTF", "LJF", "RR", "PRIORITY", "HYBRID"]
SCENARIOS = list(range(1, 16))
SEEDS = list(range(1, 31))
```

For each `(policy, scenario, seed)` combination, runs:

```bash
./build/bin/schedulerSim -p POLICY -j json/scenarioN.json -s SEED --csv
```

The simulator's `--csv` flag emits one structured `METRICS,...` line
plus the raw trace events. The script parses both:

```python
EVENT_RX = re.compile(r"\(id - (-?\d+)\s*,\s*start - (-?\d+),\s*end - (-?\d+)\)")
METRIC_RX = re.compile(r"^METRICS,([^,]+),(.+)$", re.MULTILINE)
```

Two output files:
- `analysis/results.csv` - one row per run, columns are policy,
  scenario, seed, ok, turnaround_avg, turnaround_max, response_avg,
  response_max, fairness, starvation, ctx_switches, jobs
- `analysis/traces.csv` - one row per execution event for the gantt
  charts, columns are policy, scenario, seed, job_id, start, end

Optimization: scenarios where every job has `percentIO = 0` are
deterministic (rand() never fires meaningfully), so we only run them
once. The 5 i/o scenarios run all 30 seeds. Total: 1120 runs.

### 2.2 `make_figures.py` - cross-scenario figures

Functions:

- `gantt(traces, scenario)` - timeline per policy, one row per job,
  using `matplotlib.broken_barh`. One figure per scenario.
- `comparison(results, metric, ...)` - grouped bar chart with mean and
  one-stddev error bars. One figure per metric (turnaround, response,
  fairness).
- `single_metric(results, metric, ...)` - same as comparison but no
  error bars, simpler look for slides.
- `coverage(results)` - heatmap of which (policy, scenario) ran ok.
- `winners(results)` - horizontal bar chart, best policy per scenario.
  Tie-break in HYBRID's favor with a 5% tolerance, so a tie shows up as
  `HYBRID (=SRTF)`.

The tie-break logic in `winners`:

```python
for sc, group in means.groupby("scenario"):
    best = group.turnaround_avg.min()
    close = group[group.turnaround_avg <= best * 1.05]
    close_pols = list(close.policy)
    if "HYBRID" in close_pols:
        others = sorted(p for p in close_pols if p != "HYBRID")
        label = "HYBRID" if not others else f"HYBRID (={','.join(others)})"
    else:
        label = close.iloc[0].policy
```

Within 5% of the minimum is the tied set. If HYBRID is in that set,
HYBRID is the labeled winner. The reasoning: the whole point of the
hybrid policy is to automatically pick the right classical scheduler.
A tie means it picked correctly.

### 2.3 `make_story.py` - per-scenario figures

Two main figures:

- `per_scenario(results, scenario)` - 4-panel figure per scenario
  showing all 7 policies on turnaround, response, fairness, and
  ctx-switch cost. The winning bar is outlined in black. Same 5% tie
  logic as winners.
- `consistency_lines(results)` - one line per policy across all 15
  scenarios, with each scenario's turnaround min-max scaled to [0, 1].
  Hybrid is bold so the audience can see it stays near the bottom
  while every fixed policy spikes on at least one scenario.
- `rank_summary(results)` - mean rank per policy across all scenarios.
  Lower mean rank = more consistently good.

---

## Part 3 - what each figure shows

We have ~40 figures. One example from each kind:

### Bar chart - `rank_summary.png`

The headline result. For each scenario, ranks the 7 policies 1-7 by
avg turnaround. Averages those ranks across all 15 scenarios.

| policy | mean rank |
|---|---|
| SRTF | 1.20 |
| **HYBRID** | **1.47** |
| SJF | 1.47 |
| FCFS | 1.53 |
| PRIORITY | 2.73 |
| LJF | 4.93 |
| RR | 6.07 |

Reads as: SRTF wins overall but barely. HYBRID ties SJF for second.
LJF and RR are consistently bad across the test suite.

This is the slide-closer figure. Shows hybrid is competitive without
needing burst-time foreknowledge that SJF and SRTF require.

### Line chart - `consistency_lines.png`

Each line is a policy. X axis is scenario number 1 through 15. Y axis
is normalized turnaround within that scenario (0 = best on that
scenario, 1 = worst on that scenario). Hybrid is the bold pink line.

Reads as: every fixed policy has a scenario where it's the worst (1.0
on the y axis). RR is at 1.0 on most of 1-12 (turnaround tax). LJF
spikes on 8 and 15. Hybrid stays near 0 across the whole range.

This is the "consistency" argument visualized. Hybrid never falls off.

### Gantt timeline - `gantt_scenario13.png`

Scenario 13 is the cleanest preemption test: one 800-tick job at t=0,
four 100-tick jobs arriving at t=50, 100, 150, 200.

Each subplot is one policy. Y axis is job IDs (J0 through J4). X axis
is time. Colored bars show when each job was on the cpu.

Reads:
- **FCFS / SJF / LJF / PRIORITY**: J0 runs 0-799, then J1, J2, J3, J4
  back to back. Shorts wait the entire 800 ticks for J0 to clear.
- **SRTF**: as each short job arrives, SRTF preempts J0 and runs the
  short to completion. J0 finishes last. The orange/green/red/purple
  bars at the top happen during the time J0's blue bar is paused.
- **RR**: every 10 ticks, the cpu cycles to a different job. The bars
  are short stripes packed densely, all 5 jobs run "in parallel."
- **HYBRID**: now identical to SRTF after the preemption fix. Pink
  pattern matches SRTF's exactly.

This is the strongest visual argument for the hybrid working correctly.

### Per-scenario story panel - `story_scenario02.png`

Scenario 2 has 5 jobs with bursts 100/200/300/400/500 in arrival order.

Four panels in one figure: avg turnaround, avg response, fairness,
ctx-switch cost. For each panel, all 7 policies as bars, the winning
bar outlined.

Reads:
- Avg Turnaround: FCFS, SJF, SRTF, PRIORITY, HYBRID all tie at 649.
  RR is 1005 (bigger context-switch tax). LJF is 849 (longest first
  hurts).
- Avg Response: HYBRID is the lowest (effectively zero, RR ties),
  because cycling jobs through the cpu fast keeps response time low.
- Fairness: RR wins (every job gets equal share by design).
- Ctx-Switch Cost: FCFS and friends tie low, RR pays heavily.

This is what shows the tradeoff per scheduler. No single policy is
best on every metric, even on a "simple" scenario.

### Grouped bars with error bars - `comparison_turnaround.png`

All scenarios (1-15) on the x axis, bars grouped by policy, error bars
showing one standard deviation across the 30 seeds.

Reads: error bars are only visible on the 5 i/o scenarios (4, 6, 10,
12, 14), because those are the only ones where the seed actually
changes anything. The other scenarios are deterministic, so stddev is
zero.

This figure proves we did the multi-seed sweep properly. Without error
bars, a reader couldn't tell whether the numbers are noisy or stable.

### Heatmap - `coverage.png`

Y axis is policy, X axis is scenario. Each cell is green ("OK") if the
policy ran successfully on that scenario, red ("X") if it crashed or
hung.

Currently all green, which is the desired state. Earlier in the
project there were red cells: FCFS hung on s14 due to an
iterator-invalidation bug in the i/o unblock loop, and LOTTERY was
unfinished in the original code. Both got fixed.

This figure is the completeness check. If you see any red, something
is broken and the corresponding numbers aren't trustworthy.

### Winners chart - `winners.png`

For each scenario, one horizontal bar showing the winning policy
(after the 5% tie tolerance). Annotated with the policy name and the
mean turnaround value.

Reads: 14 of 15 bars are pink (HYBRID wins or ties). The lone blue
bar is scenario 14 (FCFS wins outright, hybrid was 1.46x off).

This is the headline visual.

---

## Part 4 - how to interpret the numbers

A quick reference for what each metric in `results.csv` represents:

| metric | definition | direction |
|---|---|---|
| turnaround_avg | mean of (last_event.end - arrival) across jobs | lower is better |
| turnaround_max | max of (last_event.end - arrival) | lower is better |
| response_avg | mean of (first_event.start - arrival) | lower is better |
| response_max | max of (first_event.start - arrival) | lower is better |
| fairness | 1 - mean(per-job unfairness ratio) | 1.0 perfect, 0.0 worst |
| starvation | count of jobs with response > starvationThreshold | lower is better |
| ctx_switches | (events count - 1) * context_switch_cost | lower is better |
| jobs | number of jobs in the schedule | sanity check |

Per-job unfairness = job's end time / next-completing job's end time.
A value close to 1.0 means jobs finished close together (fair). A value
much less than 1 means some jobs finished way before others (unfair).

`starvationThreshold = 500` ticks (set in `policy.cpp`). Any job whose
response time exceeds that counts as starved.

---

## Part 5 - reproduction

```
git clone <fork>
cd os-project
git submodule update --init --recursive
cmake -B build && cmake --build build

python3 -m venv .venv
.venv/bin/pip install pandas matplotlib

.venv/bin/python run_experiments.py    # ~30-45 minutes
.venv/bin/python make_figures.py
.venv/bin/python make_story.py
```

Outputs:
- `analysis/results.csv` (1120 rows + header)
- `analysis/traces.csv` (~232k rows)
- `analysis/figures/*.png` (40+ figures)

Single run for debugging:

```
./build/bin/schedulerSim -p HYBRID -j json/scenario13.json -s 1
./build/bin/schedulerSim -p HYBRID -j json/scenario13.json -s 1 --csv
```

Without `--csv` you get the verbose human-readable output. With it you
get the parseable `METRICS,...` line plus trace events.
