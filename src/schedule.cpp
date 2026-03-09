#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "schedule.h"

void fromInput(std::vector<local::Job> schedule, std::vector<local::Job>::iterator it, int length, int number)
{
    it = schedule.begin();
   
    for(int location = 0; location < number; location++)
    {
        it = schedule.insert(it, local::Job(0, length));
    }
}

int fromJson(std::vector<local::Job> schedule, std::vector<local::Job>::iterator it, std::string filepath) //return 0 on file read, 1 on failed read
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

void printSchedule(std::vector<local::Job> schedule)
{
    for(auto j : schedule)
    {
        std::cout << j.jobString();
    }
}

