#include "fcfs.h"

#include <iostream>
#include <string>
using namespace local;

void addEvent(strategy::Trace trace, strategy::Event e)
{
    trace.trace.push_back(e);
    return;
}

strategy::Strategy evaluate(strategy::Strategy self, Schedule s) 
{
    if(self.name == "FCFS")
    {
        return strategy::FCFS::evaluate(s);
    }

    return strategy::Strategy("fake", strategy::Trace(), 0);
}

std::string toString(strategy::Event e)
{
    std::string s = "(id - " + std::to_string(e.getID()) + " , start - " + std::to_string(e.getStart());
    s += ", end - " + std::to_string(e.getEnd()) + ")";
    return s;
}

std::string toString(strategy::Trace trace)
{
    std::string s;
    for(strategy::Event e : trace.trace)
    {
        s += toString(e) + "\n";
    }
    return s;
}

std::string toString(strategy::Strategy s)
{
    std::string str;
    str += s.name + "\n";
    str += toString(s.trace) + "\n";
    return str;
}

void printTraceAnalysis(strategy::Strategy self)
{
    std::string str = toString(self);
    std::cout << str << std::endl;

    //tests past here
    str = std::string(); //clear the string for reuse
    int a;
    
    return;
}
