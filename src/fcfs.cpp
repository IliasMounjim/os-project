#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include "schedule.h"
#include "policy.h"
#include "fcfs.h"

using namespace local;

unsigned int const ioLengthRange = 100; //max io length

bool comp(Job a, Job b) //compares arrival so it goes first come first served
{
    return a.getArrival() < b.getArrival();
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
    std::uint64_t breakStart = 0;
    policy::Trace trace = policy::Trace();

    int itemNumber = 0;

    while(!readyQueue.schedule.empty() || !blockedQueue.schedule.empty()) //we're going until we work through all viable jobs
    {
        if(!running.getStarted() && !noRunning) //if not started, start, and only if we've got something to start
        { //second trigger shoudl never matter, don't want to test
            running.setStarted(true); //a gate to make sure we know only when the start is
            running.setStart(currTime);
        }

        if(bounded_rand(100) < running.getPercentIO() && !noRunning) //if we got an io moment and we've got a running job
        {
            trace.addEvent(policy::Event(running.getStart(), currTime, running.getID())); //the job ran until now
            running.setStarted(false); //the job is no longer running
            running.setIOEnd((int) (bounded_rand(ioLengthRange) + currTime)); //set ioEnd to the right number 
            blockedQueue.schedule.push_back(running); //add the blocked job to the correct queue
            if(!readyQueue.schedule.empty() && readyQueue.schedule.front().getArrival() <= currTime) //if we still have readied up jobs that have arrived
            {
                running = readyQueue.schedule.front(); //push the next to run to run
                readyQueue.schedule.erase(readyQueue.schedule.begin()); //the running is no longer ready
            }
            else //set running to a temp, what was running still has work to do
            {
                running = Job(-1, 0, 0, 0, 1);
                noRunning = true; //we've got nothing to run from here
            }
        }

        if(!blockedQueue.schedule.empty())
        {
            for(std::vector<Job>::iterator it = blockedQueue.schedule.begin(); it != blockedQueue.schedule.end(); it++) //for each blocked job
            { //blocked to ready
                if(currTime == (*it).getIOEnd()) //if the job at it is done with i/o
                {
                    readyQueue.schedule.push_back(*it);
                    blockedQueue.schedule.erase(it);
                    if(blockedQueue.schedule.empty()) //makes sure erasing it doesn't cause issues
                    {
                        break;
                    }
                    it = blockedQueue.schedule.begin(); //won't go out of bounds
                }
            }
        }

        if(!noRunning) //if we're running a job
        {
            running.decrementLength(); // the current running is closer to over
        }

        if(running.getLength() == 0) //if currently running is done
        {
            if(!noRunning) //if we were runnign to begin with
            {
                running.setStatus(1); //set running to done
                trace.addEvent(policy::Event(running.getStart(), currTime, running.getID())); //add the relevant event to trace
                std::sort(readyQueue.schedule.begin(), readyQueue.schedule.end(), comp); //sort by arrival, it's first come first served
            }
            if(!readyQueue.schedule.empty() && readyQueue.schedule.front().getArrival() <= currTime) //if there's something to run that has arrived already
            {
                running = readyQueue.schedule.front(); //ready to running
                readyQueue.schedule.erase(readyQueue.schedule.begin()); //running out of ready
                if(noRunning)
                {
                    trace.addEvent(policy::Event(breakStart, currTime, -1));
                }
                noRunning = false;
            }
            else //there's nothing to run
            {
                noRunning = true;
                breakStart = currTime;
            }
        }
        
        currTime++;

        if(currTime % 500 == 0) //in case it takes a while to run, shows something and gives you a bit of info
        {
            if(currTime % 100000 == 0)
            {
                itemNumber = readyQueue.schedule.size() + blockedQueue.schedule.size();
            }
            std::cout << "... " << std::to_string(currTime) << " c:" << std::to_string(itemNumber) << " l:" << std::to_string(running.getLength()) << " r:" << std::to_string(noRunning) << "\r";
        }
    }

    return trace;
}

policy::Policy policy::FCFS::evaluate(Schedule s)
{
    return policy::Policy("FCFS", runJobs(s), 0);
}
