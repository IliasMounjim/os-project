#include <iostream>
#include <string>
#include <vector>
#include "policy.h"
#include "schedule.h"

using namespace local;

int starvationThreshold = 500; // adjustable starvation threshold

void policy::Trace::addEvent(policy::Event e)
{
    policy::Trace t = *this;
    t.trace.push_back(e);
    *this = t;
    return;
}

policy::Event policy::Trace::getEvent(int i)
{
    return this->trace.at(i);
}

policy::Event policy::Trace::getLastOccured(int id) // get last event tied to ID
{
    for(std::vector<policy::Event>::reverse_iterator it = this->trace.rbegin(); it < this->trace.rend(); it++)
    {
        if((*it).getID() == id)
        {
            return *it;
        }
    }

    exit(3);
}

policy::Event policy::Trace::getFirstOccured(int id) // get first event tied to id
{
    for(std::vector<policy::Event>::iterator it = this->trace.begin(); it < this->trace.end(); it++)
    {
        if((*it).getID() == id)
        {
            return *it;
        }
    }

    exit(3);
}

float policy::Trace::fairnessEvent(int id)
{ // spits out the unfairness metric based on what job's end we're looking for
    std::vector<policy::Event>::reverse_iterator idEndIt;
    std::vector<policy::Event>::iterator nextEndIt;

    for(idEndIt = this->trace.rbegin(); idEndIt != this->trace.rend(); idEndIt++)
    {
        if((*idEndIt).getID() == id)
        {
            nextEndIt = idEndIt.base();
            break;
        }
    }

    if(nextEndIt == this->trace.end())
    { // if idEndIt is the final event
        return 1;
    }

    for(; nextEndIt != this->trace.end(); nextEndIt++)
    { // find the ending after
        if((*nextEndIt).getID() != -1)
        {
            if((*nextEndIt) == (this->getLastOccured((*nextEndIt).getID())))
            {
                break;
            }
        }
    }

    return ((float) (*idEndIt).getEnd()) / ((float) (*nextEndIt).getEnd());
}

std::string toString(policy::Event e)
{
    std::string s = "(id - " + std::to_string(e.getID()) + " , start - " + std::to_string(e.getStart());
    s += ", end - " + std::to_string(e.getEnd()) + ")";
    return s;
}

std::string toString(policy::Trace trace)
{
    std::string s;
    for(policy::Event e : trace.trace)
    {
        s += toString(e) + "\n";
    }
    return s;
}

std::string toString(policy::Policy s)
{
    std::string str;
    str += s.name + "\n";
    str += toString(s.trace) + "\n";
    return str;
}

void policy::Policy::printTraceAnalysis()
{
    std::string str = toString(*this);
    std::cout << str << std::endl;

    this->trace.s.printSchedule();

    //tests past here
    int avgTurn = 0;        // average turnabout time
    int maxTurn = 0;        // maximum turnabout time
    int avgResp = 0;        // average response time
    int maxResp = 0;        // maximum response time
    int starvation = 0;     // amount of startvation
    int contextSwitches = 0;// context switch overhead
    float avgFairness = 0;  // fairness 

    int contextSwitchTime = 10;
    contextSwitches = this->trace.trace.size() - 1; // number of times context siwtched
                                    
    int turn;
    int resp;
    int starved = 0;
    float unfairness;
    
    for(Job j : this->trace.s.schedule)
    {
        resp = this->trace.getFirstOccured(j.getID()).getStart() - j.getArrival();
        turn = this->trace.getLastOccured(j.getID()).getEnd() - j.getArrival(); // end - arrival, get end from trace
        unfairness = this->trace.fairnessEvent(j.getID());
        
        if(maxResp < resp)
        {
            maxResp = resp;
        }

        if(maxTurn < turn)
        {
            maxTurn = turn;
        }

        if(this->trace.getFirstOccured(j.getID()).getStart() - j.getArrival() >= starvationThreshold)
        {
            starved = 1;
        }
        
        std::cout << "Job " << j.getID() << " - Response Time: " << resp << ", Turnabout Time: " << turn << ", Unfairness: " << unfairness << ", Starved: " << starved <<std::endl; 
        
        avgResp += resp; //add together the response times
        avgTurn += turn; //add together all turnabouts
        avgFairness += unfairness; //add together all

        if(starved)
        {
            starvation++;
        }

        starved = 0;
    }

    avgTurn = avgTurn / this->trace.s.schedule.size(); //take the average
    avgResp = avgResp / this->trace.s.schedule.size(); //take the average
    avgFairness = avgFairness / this->trace.s.schedule.size();
    
    std::cout << std::endl;
    std::cout << "Turnabout time - " << "(Average: " << avgTurn << ", Maximum: " << maxTurn << ")" << std::endl;
    std::cout << "Response time - " << "(Average: " << avgResp << ", Maximum: " << maxResp << ")" << std::endl;
    std::cout << "Fairness: " << avgFairness << std::endl;
    std::cout << "Instances of Starvation: " << starvation << std::endl;
    std::cout << "Overhead from Context Switches: " << contextSwitches * contextSwitchTime << std::endl;
    
    return;
}
