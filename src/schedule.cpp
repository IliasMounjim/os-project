#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "schedule.h"

using namespace local;

void fromInput(std::vector<Job> schedule, int length, int number, int percentIO)
{
    std::vector<Job>::iterator it = schedule.begin();
   
    for(int id = 0; id < number; id++)
    {
        it = schedule.insert(it, Job(id, percentIO, length, number, 1));
    }
}

int fromJson(std::vector<Job> schedule, std::string filepath, int percentIO) //return 0 on file read, 1 on failed read
{ //TODO: FINISH JSON INPUT
    std::vector<Job>::iterator it = schedule.begin();
    
    std::ifstream json;
    json.open(filepath);
    
    if(json.is_open())
    {
        int i;
        return 0;
    }
    
    std::cout << "Couldn't read file, try again";
    std::exit(1);
}

void printSchedule(std::vector<Job> schedule)
{
    for(auto j : schedule)
    {
        std::cout << j.jobString();
    }
}
