// ========================================================================
// lottery.cpp
// ------------------------------------------------------------------------
// Description: Ticket-based randomized scheduler. Each arrived job gets
//   tickets equal to (priority + 1), then on every scheduling decision
//   we draw a random ticket and run the job that owns it. Preemptive
//   with a quantum, same shape as RR.
//
//   Original code in this file was unfinished: runJobs called .front()
//   on an empty queue (never bootstrapped readyQueue), and the ticket
//   logic was missing (totalTickets was declared but never used).
//   Rewrote so it actually links and produces sensible numbers.
// ------------------------------------------------------------------------
// Author: Illiasse Mounjim
// Version: 2026.04.27
// ========================================================================

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include "schedule.h"
#include "policy.h"
#include "lottery.h"

using namespace local;

unsigned int const ioLengthRange = 100; // max i/o duration in ticks

static unsigned bounded_rand(unsigned range)
{ // as printed in cppreference on the std::rand page
    if (range == 0) return 0;
    for (unsigned x, r;;)
        if (x = rand(), r = x % range, x - r <= -range)
            return r;
}

// give each job a number of tickets weighted by priority. higher priority
// gets more tickets so it's more likely to win the draw.
static int ticketsFor(Job &j) { return j.getPriority() + 1; }

// run a ticket draw over arrived jobs in the ready queue. returns the
// index of the winner, or -1 if nothing is eligible.
static int lotteryDraw(Schedule &q, std::uint64_t t)
{
    int total = 0;
    for (Job j : q.schedule)
        if (j.getArrival() <= (int)t)
            total += ticketsFor(j);

    if (total <= 0) return -1;

    int winner = (int)bounded_rand((unsigned)total);
    int sum    = 0;
    for (int i = 0; i < (int)q.schedule.size(); i++)
    {
        if (q.schedule[i].getArrival() <= (int)t)
        {
            sum += ticketsFor(q.schedule[i]);
            if (sum > winner) return i;
        }
    }
    return -1;
}

// pull the winning job out of ready and close any open idle gap
static Job lotteryNextJob(Schedule &readyQueue, std::uint64_t currTime,
                          bool &noRunning, std::uint64_t breakStart,
                          policy::Trace &trace)
{
    int idx = lotteryDraw(readyQueue, currTime);
    if (idx >= 0)
    {
        Job next = readyQueue.schedule[idx];
        readyQueue.schedule.erase(readyQueue.schedule.begin() + idx);
        if (noRunning){
            trace.addEvent(policy::Event(breakStart, currTime, -1));
            breakStart = 0;
        }
        noRunning = false;
        next.setStarted(true);
        next.setStart(currTime);
        return next;
    }
    noRunning = true;
    return Job(-1, 0, 0, 0, 1);
}

// main loop. tick-based, preemptive on quantum boundaries like RR.
policy::Trace lotteryRunJobs(Schedule s, int quantum)
{
    Schedule readyQueue   = Schedule(s);
    Schedule blockedQueue = Schedule();
    policy::Trace trace   = policy::Trace();

    trace.s = s;

    std::sort(readyQueue.schedule.begin(), readyQueue.schedule.end(),
        [](Job a, Job b) { return a.getArrival() < b.getArrival(); });

    Job running    = readyQueue.schedule.front();
    readyQueue.schedule.erase(readyQueue.schedule.begin());
    bool noRunning  = false;
    std::uint64_t currTime   = 0;
    std::uint64_t breakStart = 0;
    int           slice      = 0;

    while (!readyQueue.schedule.empty() || !blockedQueue.schedule.empty()
           || (!noRunning && running.getLength() > 0))
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

        // job done
        if (running.getLength() <= 0)
        {
            if (!noRunning)
            {
                running.setStatus(1);
                trace.addEvent(policy::Event(running.getStart(), currTime,
                                             running.getID()));
            }
            running = lotteryNextJob(readyQueue, currTime, noRunning,
                                     breakStart, trace);
            if (noRunning && breakStart == 0) breakStart = currTime;
            slice = 0;
        }
        // quantum expired, redraw
        else if (slice >= quantum && !noRunning)
        {
            trace.addEvent(policy::Event(running.getStart(), currTime,
                                         running.getID()));
            running.setStarted(false);
            readyQueue.schedule.push_back(running);

            running = lotteryNextJob(readyQueue, currTime, noRunning,
                                     breakStart, trace);
            if (noRunning && breakStart == 0) breakStart = currTime;
            slice = 0;
        }

        // i/o block
        if (bounded_rand(100) < (unsigned)running.getPercentIO() && !noRunning)
        {
            trace.addEvent(policy::Event(running.getStart(), currTime,
                                         running.getID()));
            running.setStarted(false);
            running.setIOEnd((int)(bounded_rand(ioLengthRange) + currTime));
            blockedQueue.schedule.push_back(running);

            running = lotteryNextJob(readyQueue, currTime, noRunning,
                                     breakStart, trace);
            if (noRunning && breakStart == 0) breakStart = currTime;
            slice = 0;
        }

        // unblock i/o-finished jobs
        for (auto it = blockedQueue.schedule.begin();
             it != blockedQueue.schedule.end(); )
        {
            if (currTime == (std::uint64_t)(*it).getIOEnd())
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

policy::Policy policy::LOTTERY::evaluate(Schedule s)
{
    return policy::Policy("LOTTERY", lotteryRunJobs(s, quantum), quantum);
}
