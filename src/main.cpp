#include <iostream>
#include <ostream>
#include <string>
#include <vector>
#include "schedule.h"

int main(int argc, char **args)
{
    std::vector<std::string> arg;
    std::string strategies[] = {"FCFS", "LJF", "SJF", "SRTF", "RR", "LOTTERY", "HYBRID"};
    std::string usageOut = "Improper use, valid forms as follows\n"
        "schedulerSim strategy length number\n"
        "schedulerSim strategy json\n"
        "schedulerSim help <COMMAND>";   

    for(int i = 0; i < argc; i++)
    {
        arg.push_back(args[i]);
    }

    for(auto a: arg)
    {
        std::cout << a << std::endl;
    }
    
    if((argc == 2 && arg[1] == "help") || (argc == 3 && arg[1] == "help" && arg[2] == "help"))
    { //hellp or help help
        std::cout << usageOut << std::endl;
        std::cout << "schedulerSim help <COMMAND>\n"
            "strategy - strategy from list: fcfs, ...\n"
            "length - length of jobs\n"
            "number - number of jobs\n"
            "json - json filepath for job set" 
            << std::endl;
        
        return 0;
    }
    else
    { 
        std::cout << usageOut;
        
        return 0;
    }
}
