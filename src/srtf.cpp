#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "schedule.h"
#include "policy.h"
#include "srtf.h"

using namespace local;

// Maximum IO Length
unsigned int const IO_LENGTH_RANGE = 100;

// Compare job length, returning true if a is shorter than b
bool srtf_compare_length(Job& a, Job& b) {
    return a.getLength() < b.getLength();
}

// Compare job arrival, returning true if a arrived before b
bool srtf_compare_arrival(Job& a, Job& b) {
    return a.getArrival() < b.getArrival();
}

Job srtf_get_next_job(Schedule& s) {
    Job next_job = s.schedule.front();
    for(std::vector<Job>::iterator it = s.schedule.begin(); it != s.schedule.end(); it++) {
        // If the job has a shorter length, select it
        if(srtf_compare_length(*it, next_job)) {
            next_job = *it;

        // If the same length, compare arrival time
        } else if (it->getLength() == next_job.getLength()) {
            if(srtf_compare_arrival(*it, next_job)) {
                next_job = *it;
            }
        }
    }

    return next_job;
}

// Generate a random number in the range [0, range)
// Same method as implemented in FCFS.cpp
unsigned srtf_bounded_rand(unsigned range) {
    for (unsigned x, r;;)
        if (x = rand(), r = x % range, x - r <= -range)
            return r;
}

policy::Trace srtfRunJobs(Schedule s) {

    // Initialize the three queues
    Schedule unarrivedQueue = Schedule(s); // Jobs that have not yet arrived
    Schedule readyQueue = Schedule(); // Jobs that are ready to run
    Schedule blockedQueue = Schedule(); // Jobs that are blocked for I/O

    // Retrieve the number of jobs and track finished jobs
    int total_jobs = s.schedule.size();
    int finished_jobs = 0;

    // Currently running job, set to a dummy job
    Job running = Job(-1, 0, 0, 0, 1);
    
    // Track if we have a running job or not
    bool job_running = false;
                                                           
    std::uint64_t current_time = 0;
    std::uint64_t break_start = 0;

    // Initialize trace to record events and return at end
    policy::Trace trace = policy::Trace();
    trace.s = s;

    // Used for printing info
    int itemNumber = 0;

    // While not all jobs are finished
    while(finished_jobs < total_jobs) {

        // Prevent integer overflow of timer
        if(current_time == UINT64_MAX){
            std::cout << "Ran too long, terminating" << std::endl;
            exit(2);
        }

        // Retrieve arrived jobs and add to ready queue
        bool preempt = false;
        for (std::vector<Job>::iterator it = unarrivedQueue.schedule.begin(); it != unarrivedQueue.schedule.end();) {
            // Check if the current job has arrived
            if (it->getArrival() <= current_time) {

                // Add the job to the ready queue
                readyQueue.schedule.push_back(*it);

                // Check if the newly arrived job has a shorter length
                if (job_running && srtf_compare_length(*it, running)) {
                    preempt = true;
                }

                // Remove the job from the unarrived queue
                it = unarrivedQueue.schedule.erase(it);
            
            } else {
                it++;
            }
        }

        // If no jobs are available to run
        if(job_running == false && readyQueue.schedule.empty() == true) {
            job_running = false;
            break_start = current_time;
        }
        
        // If no job is currently running, select a job from the ready queue
        if(job_running == false) {
            if (readyQueue.schedule.empty() == false) {
                // Add the break to trace
                trace.addEvent(policy::Event(break_start, current_time));

                // Retrieve the next job to run from the ready queue
                running = srtf_get_next_job(readyQueue);
                job_running = true;

                // Start the next job
                running.setStarted(true);
                running.setStart(current_time);

                // Remove the job from the ready queue
                for (std::vector<Job>::iterator it = readyQueue.schedule.begin(); it != readyQueue.schedule.end();) {
                    if (it->getID() == running.getID()) {
                        it = readyQueue.schedule.erase(it);
                    } else {
                        it++;
                    }
                }

            }
        }

        // If preempting the current job
        if(preempt == true) {
            if (readyQueue.schedule.empty() == false) {

                // Add the preempted job back to the ready queue
                readyQueue.schedule.push_back(running);

                // Retrieve the next job to run from the ready queue
                running = srtf_get_next_job(readyQueue);
                job_running = true;

                // Start the next job
                running.setStarted(true);
                running.setStart(current_time);

                // Remove the job from the ready queue
                for (std::vector<Job>::iterator it = readyQueue.schedule.begin(); it != readyQueue.schedule.end();) {
                    if (it->getID() == running.getID()) {
                        it = readyQueue.schedule.erase(it);
                    } else {
                        it++;
                    }
                }

            }
        }
        
        // If there is a job running and we are not preempting it
        if(job_running == true && preempt == false) {
            running.decrementLength();
        }

        // If the running job has finished
        if(running.getLength() <= 0) {
            if(job_running == true) {
                running.setStatus(1); // Set to finished
                trace.addEvent(policy::Event(running.getStart(), current_time, running.getID())); // Add finish event to trace
                finished_jobs++; // Increment finished job counter

                // Set running to a dummy job
                running = Job(-1, 0, 0, 0, 1);
                job_running = false;
            }
        }
        
        // If the job has an IO event
        if(job_running == true && srtf_bounded_rand(100) < running.getPercentIO()) {

            trace.addEvent(policy::Event(running.getStart(), current_time, running.getID())); // Add event to trace
            running.setStarted(false); // Job not running while blocked
            running.setIOEnd((int) (srtf_bounded_rand(IO_LENGTH_RANGE) + current_time)); // Set IO end time
            blockedQueue.schedule.push_back(running); // Add the blocked job to the correct queue

            // Set running to a dummy job
            running = Job(-1, 0, 0, 0, 1);
            job_running = false;

        }

        // If there are jobs in the blocked queue
        if(!blockedQueue.schedule.empty()) {

            for(std::vector<Job>::iterator it = blockedQueue.schedule.begin(); it != blockedQueue.schedule.end();) {
                // Check if the job is done with IO
                if(current_time >= it->getIOEnd()) {
                    // Add job back to the ready queue
                    readyQueue.schedule.push_back(*it);
                    it = blockedQueue.schedule.erase(it);
                } else {
                    it++;
                }
            }
        }

        // Increment time
        current_time++;

        if(current_time % 500 == 0) //in case it takes a while to run, shows something and gives you a bit of info
        {
            if(current_time % 100000 == 0)
            {
                itemNumber = readyQueue.schedule.size() + blockedQueue.schedule.size();
            }
            std::cout << "... " << std::to_string(current_time) << " c:" << std::to_string(itemNumber) << " l:" << std::to_string(running.getLength()) << " r:" << std::to_string(!job_running) << "\r";
        }
    }

    return trace;
}

policy::Policy policy::SRTF::evaluate(Schedule s) {
    return policy::Policy("SRTF", srtfRunJobs(s), 0);
}
