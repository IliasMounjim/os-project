#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "schedule.h"

using namespace local;

void Schedule::fromInput(int lengthLeft, int number, int percentIO)
{
    std::vector<Job> s = this->schedule;
   
    for(int id = 0; id < number; id++)
    {
        s.push_back(Job(id, percentIO, 0, lengthLeft, 0));
    }
    
    this->schedule = s;
}

int Schedule::fromJson(std::string filepath, int percentIO) //return 0 on file read, 1 on failed read
{ //TODO: FINISH JSON INPUT
    std::vector<Job> s = this->schedule;
    std::vector<Job>::iterator it = s.begin();
    
    std::ifstream json;
    json.open(filepath);
    
    if(json.is_open())
    {
        this->schedule = s;
        
        return 0;
    }
     
    std::cout << "Couldn't read file, try again";
    std::exit(1);
}

void Schedule::printSchedule()
{
    for(auto j : this->schedule)
    {
        std::cout << j.jobString() << std::endl;
    }
}
