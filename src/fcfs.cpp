#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include "policy.h" //carries in schedule

using namespace local;

unsigned int ioLengthRange = 100; //max io length

bool comp(Job a, Job b) //compares arrival so it goes first come first served
{
    if(a.getArrival() <= b.getArrival())
    {
        return true;
    }
    return false;
}

unsigned bounded_rand(unsigned range)
{ //as printed in cppreference on the std::rand page
    for (unsigned x, r;;)
        if (x = rand(), r = x % range, x - r <= -range)
            return r;
}

policy::Trace runJobs(Schedule s)
{
    Schedule readyQueue = Schedule(s); //all jobs are added to the ready queue after having been sorted appropriatly
    Schedule blockedQueue = Schedule(); //starts empty
    Job running = Job(readyQueue.schedule.front()); //on FCFS we start running 
    readyQueue.schedule.erase(readyQueue.schedule.begin()); //running job leaves queue
    
    bool noRunning = false; //nothing running, waiting for blocked

    std::sort(readyQueue.schedule.begin(), readyQueue.schedule.end(), comp); //sort by arrival, it's first come first served, speed up runtime
                                                           
    std::uint64_t currTime = 0;
    policy::Trace trace = policy::Trace();

    while(!readyQueue.schedule.empty() && blockedQueue.schedule.empty()) //we're going until we work through all viable jobs
    {
        if(!running.getStarted() && !noRunning) //if not started, start, and only if we've got something to start
        { //second trigger shoudl never matter, don't want to test
            running.setStarted(true); //a gate to make sure we know only when the start is
            running.setStart(currTime);
        }
        
        if(bounded_rand(100) <= running.getPercentIO() && !noRunning) //if we got an io moment and we've got a running job
        {
            trace.addEvent(trace, policy::Event(running.getStart(), currTime, running.getID())); //the job ran until now
            running.setStarted(false); //the job is no longer running
            running.setIOEnd((int) (bounded_rand(ioLengthRange) + currTime)); //set ioEnd to the right number 
            blockedQueue.schedule.push_back(running); //add the blocked job to the correct queue
            running = readyQueue.schedule.front(); //push the next to run to run
            readyQueue.schedule.erase(readyQueue.schedule.begin()); //the running is no longer ready
        }

        for(std::vector<Job>::iterator it = blockedQueue.schedule.begin(); it < blockedQueue.schedule.end(); it++) //for each blocked job
        { //blocked to ready
            if(currTime == (*it).getIOEnd()) //if the job at it is done with i/o
            {
                readyQueue.schedule.push_back(*it);
                blockedQueue.schedule.erase(it);
                it = it--; //do not go out of bounds
            }
        }

        if(running.getLength() == 0) //if currently running is done and we are runnign to begin with
        {
            if(!noRunning)
            {
                running.setStatus(1); //set running to done
                trace.addEvent(trace, policy::Event(running.getStart(), currTime, running.getID())); //add the relevant event to trace
                std::sort(readyQueue.schedule.begin(), readyQueue.schedule.end(), comp); //sort by arrival, it's first come first served
            }
            if(!readyQueue.schedule.empty()) //if there's something to run
            {
                running = readyQueue.schedule.front(); //ready to running
                noRunning = false;
            }
            else //there's nothing to run
            {
                noRunning = true;
            }
        }
        
        if(!noRunning) //if we're running a job
        {
            running.decrementLength(); // the current running is closer to over
        }
        currTime++;
    }

    return trace;
}

policy::Policy evaluate(Schedule s)
{
    return policy::Policy("FCFS", runJobs(s), 0);
}
