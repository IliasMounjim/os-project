/*
 * rr.cpp — Round Robin (preemptive, time-sliced)
 *
 * Each job runs for at most `quantum` ticks, then gets preempted to the
 * back of the ready queue. Guarantees bounded wait: (n-1)*quantum.
 * Starvation-free by design, but small quantum = lots of context switches
 * and large quantum degenerates into FCFS (OSTEP ch7).
 *
 * I/O model is the same as sjf.cpp and fcfs.cpp — percentIO chance of
 * blocking each tick, random duration in [0, 100).
 */

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include "schedule.h"
#include "policy.h"
#include "rr.h"

using namespace local;

unsigned int const rrIoRange = 100; // max i/o duration in ticks

// bias-free rand in [0, range) — same rejection method as sjf/fcfs
unsigned rrRand(unsigned range)
{
    for (unsigned x, r;;)
        if (x = rand(), r = x % range, x - r <= -range)
            return r;
}

// main simulation loop for round robin.
// same tick structure as sjf, but adds quantum-based preemption.
policy::Trace rrRunJobs(Schedule s, int quantum)
{
    Schedule readyQueue   = Schedule(s);
    Schedule blockedQueue = Schedule();
    policy::Trace trace   = policy::Trace();
    
    trace.s = s;

    std::sort(readyQueue.schedule.begin(), readyQueue.schedule.end(),
        [](Job a, Job b) { return a.getArrival() < b.getArrival(); });

    Job running = readyQueue.schedule.front();
    readyQueue.schedule.erase(readyQueue.schedule.begin());

    bool          noRunning  = false;
    std::uint64_t currTime   = 0;
    std::uint64_t breakStart = 0;
    int           slice      = 0; // ticks used in current quantum

    while (!readyQueue.schedule.empty() || !blockedQueue.schedule.empty())
    {
        if (currTime == UINT64_MAX)
        {
            std::cout << "Ran too long, terminating" << std::endl;
            exit(2);
        }

        if (!running.getStarted() && !noRunning)
        {
            running.setStarted(true);
            running.setStart(currTime);
        }

        if (!noRunning)
        {
            running.decrementLength();
            slice++;
        }

        // job finished before quantum expired
        if (running.getLength() <= 0)
        {
            if (!noRunning)
            {
                running.setStatus(1);
                trace.addEvent(policy::Event(running.getStart(), currTime,
                                             running.getID()));
            }

            if (!readyQueue.schedule.empty() &&
                readyQueue.schedule.front().getArrival() <= currTime)
            {
                running = readyQueue.schedule.front();
                readyQueue.schedule.erase(readyQueue.schedule.begin());
                if (noRunning)
                    trace.addEvent(policy::Event(breakStart, currTime, -1));
                noRunning = false;
                slice = 0;
                // initialize start so a same-tick i/o block doesn't read garbage
                running.setStarted(true);
                running.setStart(currTime);
            }
            else
            {
                noRunning  = true;
                breakStart = currTime;
            }
        }
        // quantum used up — preempt, push to tail, pull from front
        else if (slice >= quantum && !noRunning)
        {
            trace.addEvent(policy::Event(running.getStart(), currTime,
                                         running.getID()));
            running.setStarted(false);
            readyQueue.schedule.push_back(running); // back of the line

            running = readyQueue.schedule.front();  // next in line
            readyQueue.schedule.erase(readyQueue.schedule.begin());
            noRunning = false;
            slice = 0;
            // same fix as above, see comment
            running.setStarted(true);
            running.setStart(currTime);
        }

        // i/o block — stochastic, same model as sjf/fcfs
        if (rrRand(100) < running.getPercentIO() && !noRunning)
        {
            trace.addEvent(policy::Event(running.getStart(), currTime,
                                         running.getID()));
            running.setStarted(false);
            running.setIOEnd((int)(rrRand(rrIoRange) + currTime));
            blockedQueue.schedule.push_back(running);

            if (!readyQueue.schedule.empty() &&
                readyQueue.schedule.front().getArrival() <= currTime)
            {
                running = readyQueue.schedule.front();
                readyQueue.schedule.erase(readyQueue.schedule.begin());
                slice = 0;
                running.setStarted(true);
                running.setStart(currTime);
            }
            else
            {
                running   = Job(-1, 0, 0, 0, 1);
                noRunning = true;
            }
        }

        // unblock jobs whose i/o finished
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

policy::Policy policy::RR::evaluate(Schedule s)
{
    return policy::Policy("RR", rrRunJobs(s, quantum), quantum);
}
