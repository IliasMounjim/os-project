Hybrid CPU Scheduler
CSCI OS, CU Denver. Illiasse Mounjim, James Osborne, Braden Tillema.


What this is
A discrete-event simulator for seven classical scheduling policies
(FCFS, SJF, SRTF, LJF, RR, Priority, plus a rule-based Hybrid) over
fifteen designed workloads. We're testing whether runtime-observable
signals (burst variance, percent I/O) are enough to make the hybrid
pick the right sub-policy without predicting future jobs.


Run it

    cmake -B build
    cmake --build build
    python3 analyze.py

That writes analysis/results.csv and seven PNGs into analysis/figures/.
Reruns are bit-identical (seed is hashed from scenario+replicate).
If results.csv is up to date, the sweep is skipped and only figures
are redrawn. Needs pandas and matplotlib.


The hybrid rule

    avg(percentIO) >= 15  ->  RR
    CoV(burst)  < 0.2     ->  FCFS
    CoV(burst)  > 0.5     ->  SJF
    else                  ->  RR

CoV is over the running job plus arrived ready queue (cold-start
matters). Re-evaluated every tick, but a mode change only sticks if
hybStabilization (250) ticks have passed since the last one. SJF
mode also preempts mid-run if a shorter eligible job is waiting.

Defaults in src/hybrid.cpp: hybCovLow=0.2, hybCovHigh=0.5,
hybIoCutoff=15, hybStabilization=250. Not retuned on the test set.


Layout

    CMakeLists.txt
    external/jsoncpp/         submodule
    src/                      one .cpp/.h pair per policy + main + schedule + policy
    json/                     scenario1.json .. scenario15.json
    analyze.py                one shot, no flags

After running:

    analysis/results.csv      one row per (policy, scenario, seed)
    analysis/figures/         fig1..fig7 PNGs at 300 DPI


Scenarios
Fifteen total. Five (s4, s6, s10, s12, s14) have nonzero percentIO so
the I/O block is stochastic; those run with 100 seeds each. The other
ten are deterministic, one seed each. 3,570 runs total. Per-scenario
parameters in json/scenarios.md.


Notes
Lottery is implemented but excluded from the figures (high seed-to-
seed variance, not enough replicates to report fairly). Hardware is
abstracted, per-tick context-switch cost is constant. HYBRID trails
pure RR on s4 and s6 (the dead zone, 0.2 < CoV < 0.5, low I/O), and
trails non-preemptive policies on s14 (the I/O branch routes to RR
which has the structural turnaround cost). Across the suite HYBRID
still wins 11 of 15 on turnaround and 12 of 15 on waiting time.
