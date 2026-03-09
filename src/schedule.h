#include <string>
#include <vector>

namespace local {
    class Job
    {
        int arrival; //arrival time
        int length;  //job length
    
        public:
            Job(int arrival, int length)
                : arrival(arrival)
                , length(length)
            {}

            std::string jobString() {return "Arrival time: " + std::to_string(arrival) + "\nJob length: " + std::to_string(length) + "\n\0";}
            int getArrival() {return arrival;}
            int getLength() {return length;}
    };
    
    class Schedule
    {
        public:
            std::vector<Job> schedule;
            std::vector<Job>::iterator it;
            
            void printSchedule(std::vector<Job> schedule);
            void fromInput(std::vector<Job> schedule, std::vector<local::Job>::iterator it, int length, int number);
            int fromJson(std::vector<Job> schedule, std::vector<local::Job>::iterator it, std::string filepath);
    
            Schedule(int length, int number)
            {
                fromInput(schedule, it, length, number);
            }

            Schedule(std::string filepath)
            {
                fromJson(schedule, it, filepath);
            }
    };
}
