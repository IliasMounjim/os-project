#include <algorithm>
#include "schedule.h"

bool comp(local::Job a, local::Job b)
{
    if(a.getArrival() <= b.getArrival())
    {
        return true;
    }
    return false;
}

void runJobs(local::Schedule s) 
{
    std::sort(s.schedule.begin(), s.schedule.end(), comp);
}

