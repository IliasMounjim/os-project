/*
 * sjf.cpp — Shortest Job First (non-preemptive)
 *
 * Picks the job with the smallest remaining burst from the ready set.
 * Optimal for avg waiting time in the non-preemptive case (OSTEP ch7),
 * but starves long jobs when short ones keep showing up.
 *
 * I/O model matches fcfs.cpp — each tick has a percentIO chance of
 * blocking, and blocked jobs sit out for a random duration in [0, 100).
 */

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <climits>
#include <string>
#include <vector>
#include "schedule.h"
#include "policy.h"
#include "sjf.h"

using namespace local;

unsigned int const sjfIoRange = 100; // max i/o duration in ticks

// bias-free rand in [0, range) — rejection sampling from cppreference
unsigned sjfRand(unsigned range)
{
    for (unsigned x, r;;)
        if (x = rand(), r = x % range, x - r <= -range)
            return r;
}

// scan the ready queue for the shortest job that has already arrived.
// returns index into q.schedule, or -1 if nothing is eligible yet.
int pickShortest(Schedule &q, std::uint64_t t)
{
    int pick   = -1;
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
    return pick;
}

// pull the shortest eligible job out of the ready queue.
// if nothing is available, sets noRunning and returns a dummy job.
// also logs idle gaps (jobID = -1) so analysis can compute utilization.
Job sjfNextJob(Schedule &readyQueue, std::uint64_t currTime, bool &noRunning,
               std::uint64_t breakStart, policy::Trace &trace)
{
    int idx = pickShortest(readyQueue, currTime);

    if (idx >= 0)
    {
        Job next = readyQueue.schedule[idx];
        readyQueue.schedule.erase(readyQueue.schedule.begin() + idx);

        if (noRunning) // cpu was idle — close the gap event
            trace.addEvent(policy::Event(breakStart, currTime, -1));

        noRunning = false;
        return next;
    }

    // nothing to run yet
    noRunning = true;
    return Job(-1, 0, 0, 0, 1);
}

// main simulation loop — tick-based discrete event sim.
// each tick: execute -> check completion -> check i/o -> unblock -> advance
policy::Trace sjfRunJobs(Schedule s)
{
    Schedule readyQueue   = Schedule(s);
    Schedule blockedQueue = Schedule();
    policy::Trace trace   = policy::Trace();

    // sort by arrival so we start with the earliest job
    std::sort(readyQueue.schedule.begin(), readyQueue.schedule.end(),
        [](Job a, Job b) { return a.getArrival() < b.getArrival(); });

    // bootstrap — first arrived job starts running immediately
    Job running = readyQueue.schedule.front();
    readyQueue.schedule.erase(readyQueue.schedule.begin());

    bool          noRunning  = false;
    std::uint64_t currTime   = 0;
    std::uint64_t breakStart = 0;

    while (!readyQueue.schedule.empty() || !blockedQueue.schedule.empty())
    {
        if (currTime == UINT64_MAX)
        {
            std::cout << "Ran too long, terminating" << std::endl;
            exit(2);
        }

        // mark the first tick this job actually starts running
        if (!running.getStarted() && !noRunning)
        {
            running.setStarted(true);
            running.setStart(currTime);
        }

        // execute one tick
        if (!noRunning)
            running.decrementLength();

        // job finished — log it, pick the next shortest
        if (running.getLength() <= 0)
        {
            if (!noRunning)
            {
                running.setStatus(1);
                trace.addEvent(policy::Event(running.getStart(), currTime,
                                             running.getID()));
            }
            running = sjfNextJob(readyQueue, currTime, noRunning,
                                 breakStart, trace);
            if (noRunning)
                breakStart = currTime;
        }

        // i/o interrupt — stochastic, same model as fcfs
        if (sjfRand(100) < running.getPercentIO() && !noRunning)
        {
            trace.addEvent(policy::Event(running.getStart(), currTime,
                                         running.getID()));
            running.setStarted(false);
            running.setIOEnd((int)(sjfRand(sjfIoRange) + currTime));
            blockedQueue.schedule.push_back(running);

            running = sjfNextJob(readyQueue, currTime, noRunning,
                                 breakStart, trace);
            if (noRunning)
                breakStart = currTime;
        }

        // move jobs whose i/o finished back to ready
        for (auto it = blockedQueue.schedule.begin();
             it != blockedQueue.schedule.end(); )
        {
            if (currTime == (*it).getIOEnd())
            {
                readyQueue.schedule.push_back(*it);
                it = blockedQueue.schedule.erase(it);
            }
            else
                it++;
        }

        currTime++;

        if (currTime % 500 == 0)
        {
            if (currTime % 100000 == 0)
                std::cout << "... t=" << currTime
                          << " remaining="
                          << readyQueue.schedule.size()
                             + blockedQueue.schedule.size()
                          << "\r";
        }
    }

    return trace;
}

policy::Policy policy::SJF::evaluate(Schedule s)
{
    return policy::Policy("SJF", sjfRunJobs(s), 0);
}
