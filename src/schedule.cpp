#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "schedule.h"

using namespace local;

void fromInput(std::vector<Job> schedule, std::vector<Job>::iterator it, int length, int number)
{
    it = schedule.begin();
   
    for(int id = 0; id < number; id++)
    {
        it = schedule.insert(it, Job(0, length, id));
    }
}

int fromJson(std::vector<Job> schedule, std::vector<Job>::iterator it, std::string filepath) //return 0 on file read, 1 on failed read
{ //TODO: FINISH JSON INPUT
    it = schedule.begin();
    
    std::ifstream json;
    json.open(filepath);
    
    if(json.is_open())
    {
        int i;
        return 0;
    }
    
    std::cout << "Couldn't read file, try again";
    return 1;
}

void printSchedule(std::vector<Job> schedule)
{
    for(auto j : schedule)
    {
        std::cout << j.jobString();
    }
}
