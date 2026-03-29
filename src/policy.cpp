#include <iostream>
#include <string>
#include "policy.h"

using namespace local;

void policy::Trace::addEvent(policy::Event e)
{
    policy::Trace t = *this;
    t.trace.push_back(e);
    *this = t;
    return;
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

    //tests past here
    str = std::string(); //clear the string for reuse
    int a;
    
    return;
}
