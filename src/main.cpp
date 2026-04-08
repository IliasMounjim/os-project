#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "fcfs.h"
#include "policy.h"
#include "sjf.h"
#include "rr.h"
#include "srtf.h"
#include "ljf.h"
#include "priority.h"

using namespace local;

// EXIT CODES
// 0 - standard exit, might still have issues, but it got to a valid end
// 1 - invalid input
// 2 - other
// 3 - you shouldn't have got here

bool isNumber(std::string s)
{
    for (char c : s)
    {
        if(!std::isdigit(c))
            return false;
    }
    return true;
}

int intUserInput(std::string s)
{
    int temp;

    try {
        temp = std::stoi(s);
    } catch (std::invalid_argument) {
        std::cout << "invalid arguement, not a number" << std::endl;
        std::exit(1);
    } catch (std::out_of_range) {
        std::cout << "invalid arguement, number too big" << std::endl;
        std::exit(1);
    }

    return temp;
}

void helpArg()
{
    std::cout << "schedulerSim <FLAGS>" << std::endl;
    std::cout << "-h, prints this help message" << std::endl;
    std::cout << "-p POLICY, select the policy evaluated from the following list, not optional, FCFS, LJF, SJF, SRTF, RR, PRIORITY, LOTTERY, HYBRID" << std::endl;
    std::cout << "-q NUMBER, the time quantum, put in a valid int, defaults to 10" << std::endl;
    std::cout << "-l NUMBER, the length of the uniform job set, put in valid int, doesn't default" << std::endl;
    std::cout << "-i NUMBER, percentIO for jobs, uniform workload" << std::endl;
    std::cout << "-n NUMBER, the number of uniform/random jobs, put in valid int, doesn't default" << std::endl;
    std::cout << "-j FILEPATH, json job input filepath, not yet implemented" << std::endl;
    std::cout << "-r SEED, seed not needed, random job workload, not yet implemented\n" << std::endl;
    
    std::cout << "minimum required input" << std::endl;
    std::cout << "schedulerSim, returns the help string" << std::endl;
    std::cout << "schedulerSim -h, return teh help string" << std::endl;
    std::cout << "schedulerSim -p POLICY -r NUMBER -n NUMBER, random workload" << std::endl;
    std::cout << "schedulerSim -p POLICY -l NUMBER -n NUMBER, uniform workload" << std::endl;
    std::cout << "schedulerSim -p POLICY -j FILEPATH, designed workloads" << std::endl;

    exit(0);
}

int main(int argc, char **args)
{
    std::vector<std::string> arg;
    std::string policies[] = {"FCFS", "LJF", "SJF", "SRTF", "RR", "LOTTERY", "PRIORITY", "HYBRID"};

    for(int i = 0; i < argc; i++) //all arguements into strings
    {
        arg.push_back(args[i]);
    }

    std::vector<std::string>::iterator policyFlag = std::find(arg.begin(), arg.end(), "-p");
    std::vector<std::string>::iterator quantumFlag = std::find(arg.begin(), arg.end(), "-q");
    std::vector<std::string>::iterator randomFlag = std::find(arg.begin(), arg.end(), "-r");
    std::vector<std::string>::iterator numberFlag = std::find(arg.begin(), arg.end(), "-n");
    std::vector<std::string>::iterator lengthFlag = std::find(arg.begin(), arg.end(), "-l");
    std::vector<std::string>::iterator ioFlag = std::find(arg.begin(), arg.end(), "-i");
    std::vector<std::string>::iterator jsonFlag = std::find(arg.begin(), arg.end(), "-j");
    std::vector<std::string>::iterator helpFlag = std::find(arg.begin(), arg.end(), "-h");

    bool isRandom = false;
    std::string policyName;
    std::string filePath;
    int quantum = 10;
    int length = 0;
    int number = 0;
    int percentIO = 0;
    int seed = (int) time(NULL);

    if(helpFlag != arg.end()) //is there help flag
    {
        helpArg();
        return 0;
    }
    
    if(policyFlag != arg.end() && (policyFlag+1) != arg.end()) //is there policy flag
    {
        if(std::find(std::begin(policies), std::end(policies), *(policyFlag+1)) != std::end(policies)) //do we have a valid policy 
        {
            policyName = *(policyFlag+1);
        }
        else //if invalid policy quit
        {
            std::cout << "invalid policy" << std::endl;
            return 1;
        }
    }
    else //if no policy quit
    {
        std::cout << "use -p POLICY to input a policy, this is nessecary" << std::endl;
        return 1;
    }

    if(jsonFlag != arg.end()) //do we have a valid json
    {
        if(((jsonFlag+1) != policyFlag || (jsonFlag+1) != quantumFlag || (jsonFlag+1) != numberFlag || (jsonFlag+1) != lengthFlag || 
                (jsonFlag+1) != randomFlag || (jsonFlag+1) != helpFlag) && (jsonFlag+1) != arg.end()) //do we ahve valid input
        {
            filePath = *(jsonFlag+1);
        }
        else
        {
            std::cout << "invalid json filepath input" << std::endl;
            return 1;
        }
    }

    if(lengthFlag != arg.end()) //do we have the flag
    {
        if(isNumber(*(lengthFlag+1)) && (lengthFlag+1) != arg.end()) //does it have valid input
        {
            length = intUserInput(*(lengthFlag+1));
        }
        else
        {
            int a;
            return 1;
        }
    }
    
    if(numberFlag != arg.end()) //do we have the flag
    {
        if(isNumber(*(numberFlag+1)) && (numberFlag+1) != arg.end()) //does it have valid input
        {
            number = intUserInput(*(numberFlag+1));
        }
        else
        {
            int a;
            return 1;
        }
    }

    if(quantumFlag != arg.end()) //do we have a valid quantum flag
    {
        if((quantumFlag+1) != arg.end()) //valid quantum
        {
            quantum = intUserInput(*(quantumFlag+1));
        }
        else //invalid quantum
        {
            std::cout << "invalid quantum flag use" << std::endl;
            return 1;
        }
    }

    if(ioFlag != arg.end())
    {
        if((ioFlag+1) != arg.end()) //valid io input
        {
            percentIO = intUserInput(*(ioFlag+1));
        }
        else //invalid number
        {
            std::cout << "invalid percentIO flag" << std::endl;
            return 1;
        }
    }

    if(randomFlag != arg.end())
    { //TODO:ACTUALLY IMPLEMENT RANDOM SCHEDULE GENERATION
        isRandom = true;
        if((randomFlag+1) != arg.end())
        {
            seed = intUserInput(*(randomFlag+1));
        }
        else
        {
            std::cout << "invalid seed" << std::endl;
            return 1;
        }
    }
    
    if(argc < 4) //argc is less than 4 because 4 is the minimum number of argements required to run
    {
        helpArg();
    }
    if(filePath.length() > 0) //if we have json input
    {
        Schedule s = Schedule(filePath);
        if(policyName == "FCFS")
        {
            policy::FCFS p = policy::FCFS(policyName, policy::Trace(), quantum);
            policy::Policy toPrint = p.evaluate(s);
            toPrint.printTraceAnalysis();
        }
        else if(policyName == "SJF")
        {
            policy::SJF p = policy::SJF(policyName, policy::Trace(), quantum);
            policy::Policy toPrint = p.evaluate(s);
            toPrint.printTraceAnalysis();
        }
        else if(policyName == "RR")
        {
            policy::RR p = policy::RR(policyName, policy::Trace(), quantum);
            policy::Policy toPrint = p.evaluate(s);
            toPrint.printTraceAnalysis();
        }
        else if(policyName == "SRTF")
        {
            policy::SRTF p = policy::SRTF(policyName, policy::Trace(), quantum);
            policy::Policy toPrint = p.evaluate(s);
            toPrint.printTraceAnalysis();
        }
        else if(policyName == "LJF")
        {
            policy::LJF p = policy::LJF(policyName, policy::Trace(), quantum);
            policy::Policy toPrint = p.evaluate(s);
            toPrint.printTraceAnalysis();
        }
        else if(policyName == "PRIORITY")
        {
            policy::Priority p = policy::Priority(policyName, policy::Trace(), quantum);
            policy::Policy toPrint = p.evaluate(s);
            toPrint.printTraceAnalysis();
        }
        else
        {}
        
        return 0;
    }
    else if(isRandom)
    {
        //TODO: WRITE UP RANDOM
    }
    else
    {
        Schedule s = Schedule(length, number, percentIO);
        if(policyName == "FCFS")
        {
            policy::FCFS p = policy::FCFS(policyName, policy::Trace(), quantum);
            policy::Policy toPrint = p.evaluate(s);
            toPrint.printTraceAnalysis();
        }
        else if(policyName == "SJF")
        {
            policy::SJF p = policy::SJF(policyName, policy::Trace(), quantum);
            policy::Policy toPrint = p.evaluate(s);
            toPrint.printTraceAnalysis();
        }
        else if(policyName == "RR")
        {
            policy::RR p = policy::RR(policyName, policy::Trace(), quantum);
            policy::Policy toPrint = p.evaluate(s);
            toPrint.printTraceAnalysis();
        }
        else if(policyName == "SRTF")
        {
            policy::SRTF p = policy::SRTF(policyName, policy::Trace(), quantum);
            policy::Policy toPrint = p.evaluate(s);
            toPrint.printTraceAnalysis();
        }
        else if(policyName == "LJF")
        {
            policy::LJF p = policy::LJF(policyName, policy::Trace(), quantum);
            policy::Policy toPrint = p.evaluate(s);
            toPrint.printTraceAnalysis();
        }
        else if(policyName == "PRIORITY")
        {
            policy::Priority p = policy::Priority(policyName, policy::Trace(), quantum);
            policy::Policy toPrint = p.evaluate(s);
            toPrint.printTraceAnalysis();
        }
        else
        {}

        return 0;
    }

    return 0;
}
