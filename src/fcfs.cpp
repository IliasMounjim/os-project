#include <algorithm>
#include <cstdint>
#include "strategy.h" //carries in schedule

using namespace local;

bool comp(Job a, Job b) //compares arrival so it goes first come first served
{
    if(a.getArrival() <= b.getArrival())
    {
        return true;
    }
    return false;
}

strategy::Trace runJobs(Schedule s)
{
    std::sort(s.schedule.begin(), s.schedule.end(), comp); //sort by arrival, it's first come first served

    std::uint64_t currTime = 0;
    strategy::Trace trace = strategy::Trace();

    for(Job j : s.schedule) //add an event to the trace for each job run
    {
        if(j.getArrival() <= currTime) //have we passed the arrival of the job yet
        {
            trace.addEvent(trace, strategy::Event((int) currTime, (int) currTime + j.getLength(), j.getID()));
            currTime += j.getLength();
        }
        else //if not, we wait for it to arrive
        {
            currTime = j.getArrival();
            trace.addEvent(trace, strategy::Event((int) currTime, (int) currTime + j.getLength(), j.getID()));
            currTime += j.getLength();
        }
    }

    return trace;
}

strategy::Strategy evaluate(Schedule s)
{
    return strategy::Strategy("FCFS", runJobs(s), 0);
}
