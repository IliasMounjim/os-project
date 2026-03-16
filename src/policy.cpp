#include <iostream>
#include <string>
#include "fcfs.h"

using namespace local;

void addEvent(policy::Trace trace, policy::Event e)
{
    trace.trace.push_back(e);
    return;
}

policy::Policy evaluate(policy::Policy self, Schedule s) 
{
    if(self.name == "FCFS")
    {
        return policy::FCFS::evaluate(s);
    }

    return policy::Policy("fake", policy::Trace(), 0);
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

void printTraceAnalysis(policy::Policy self)
{
    std::string str = toString(self);
    std::cout << str << std::endl;

    //tests past here
    str = std::string(); //clear the string for reuse
    int a;
    
    return;
}
