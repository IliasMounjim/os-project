#include <fstream>
#include <iostream>
#include <filesystem>
#include <string>
#include <vector>
#include <json/json.h>
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

int Schedule::fromJson(std::string filepath) //return 0 on file read, 1 on failed read
{
    Schedule s = *this;
    std::filesystem::path path = filepath;

    if(!std::filesystem::exists(path)) //our file exists
    {
        std::cerr << "path does not refer to an existing filesystem object, look into that" << std::endl;
        std::exit(1);
    }
    
    std::ifstream json = std::ifstream(path);
    Json::Value v;
    Json::Reader r;
    
    if(!r.parse(json, v))
    {
        std::cerr << "JSON didn't parse quite right" << std::endl;
        exit(1);
    }

    std::string iStr;
    for(int i = 0; i < v.size(); i++)
    {
        iStr = std::to_string(i);
        s.schedule.push_back(Job(i, v[iStr]["percentIO"].asInt(), v[iStr]["arrival"].asInt(), v[iStr]["length"].asInt(), 0, v[iStr]["priority"].asInt()));
    }
    
    *this = s;
    return 0;
}

void Schedule::printSchedule()
{
    for(auto j : this->schedule)
    {
        std::cout << j.jobString() << std::endl;
    }
    std::cout << std::endl;
}
