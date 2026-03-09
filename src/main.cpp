#include <iostream>
#include <ostream>
#include <string>
#include <vector>
#include "schedule.h"

int main(int argc, char **args)
{
    std::string usageOut = "Improper use, valid forms as follows\nschedulerSim strategy length number\nschedulerSim strategy json\n";
    
    std::vector<std::string> arg;
    
    for(int i = 0; i < argc; i++)
    {
        arg.push_back(args[i]);
    }

    for(auto a: arg)
    {
        std::cout << a << std::endl;
    }
    
    if(argc == 2 && (arg[1] == "help" || arg[1] == "h"))
    {
        std::cout << usageOut << std::endl;
        
        return 0;
    }
    else
    { 
        std::cout << usageOut;
        
        return 0;
    }
}
