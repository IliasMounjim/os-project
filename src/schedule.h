#include <string>
#include <vector>

namespace local {
    class Job
    {
        private:
            int arrival; //arrival time
            int length;  //job length
            int id;
    
        public:
            Job(int arrival, int length, int id)
                : arrival(arrival)
                , length(length)
                , id(id)
            {}

            std::string jobString() { return "ID: " + std::to_string(id) + "\n" + "Arrival time: " + std::to_string(arrival) + "\nJob length: " + std::to_string(length) + "\n\0"; }
            int getArrival() { return arrival; }
            int getLength() { return length; }
            int getID() { return id; }
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
