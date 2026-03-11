#include <algorithm>
#include <cctype>
#include <iostream>
#include <ostream>
#include <string>
#include <vector>
#include "strategy.h"

using namespace local;

bool isNumber(std::string s)
{
    for (char c : s)
    {
        if(!std::isdigit(c))
            return false;
    }
    return true;
}

int main(int argc, char **args)
{
    std::vector<std::string> arg;
    std::string strategies[] = {"FCFS", "LJF", "SJF", "SRTF", "RR", "LOTTERY", "HYBRID"};
    std::string usageOut = "Improper use, valid forms as follows\n"
        "schedulerSim strategy length number <FLAGS>\n"
        "schedulerSim strategy json <FLAGS>\n"
        "schedulerSim help <COMMAND>";

    for(int i = 0; i < argc; i++)
    {
        arg.push_back(args[i]);
    }

    for(auto a: arg)
    {
        std::cout << a << std::endl;
    }
    
    if(argc == 2 && arg[1] == "help")
    { //help
        std::cout << usageOut << std::endl;
        
        return 0;
    }
    if(argc == 3 && arg[1] == "help" && arg[2] == "help")
    { //help help
        std::cout << "schedulerSim help <COMMAND>\n"
            "strategy - strategy from list: fcfs, ...\n"
            "length - length of jobs\n"
            "number - number of jobs\n"
            "json - json filepath for job set\n"
            "flags - optional\n"
            "   -q <INT>, time quantum"
            << std::endl;

        return 0;
    }
    if(argc >= 3 && std::find(std::begin(strategies), end(strategies), arg[1]))
    {
        if(argc == 3)
        {
            std::cout << "JSON not yet implemented" << std::endl;
            return 0;
        }
        else if(argc == 4) // no flag length number OR JSON broken flag
        {
            if(isNumber(arg[2]) && isNumber(arg[3]))
            {
                strategy::Strategy strat = strategy::Strategy(arg[1], strategy::Trace(), 0);
                Schedule sched = Schedule(std::stoi(arg[2]), std::stoi(arg[3]));
                strat.evaluate(strat, sched);
            }
            else
            {
                std::cout << "JSON not yet implemented, -q needs a number after" << std::endl;
                return 0;
            }
        }
        else if (argc == 5) 
        {
            
        }
        else if (argc == 6) 
        {

        }
    }
    else
    { 
        std::cout << usageOut;
        
        return 0;
    }
}
