#include <string>
#include <vector>

namespace local {
    class Job //TODO: add percent io
    {
        private:
            int id;        //job id
            int percentIO; //percent i/o
            int arrival;   //arrival time
            int length;    //job length
            int status;    //job status
                           // 0 = running
                           // 1 = ready
                           // 2 = blocked
    
        public:
            Job(int id, int pio, int arrival, int length, int status)
                : id(id)
                , percentIO(pio)
                , arrival(arrival)
                , length(length)
                , status(status)
            {}

            std::string jobString() { return "ID: " + std::to_string(id) + "\n" + "Arrival time: " + std::to_string(arrival) + "\nJob length: " + std::to_string(length) + "\n\0"; }
            int setStatus(int s) { status = s; return status; }
            int getID() { return id; }
            int getPercentIO() { return percentIO; }
            int getArrival() { return arrival; }
            int getLength() { return length; }
            int getStatus() { return status; }
    };
    
    class Schedule
    {
        public:
            std::vector<Job> schedule;
            
            void printSchedule(std::vector<Job> schedule);
            void fromInput(std::vector<Job> schedule, int length, int number, int percentIO);
            int fromJson(std::vector<Job> schedule, std::string filepath, int percentIO);
    
            Schedule(int length, int number, int percentIO)
            {
                fromInput(schedule, length, number, percentIO);
            }

            Schedule(std::string filepath, int percentIO)
            {
                fromJson(schedule, filepath, percentIO);
            }
    };
}
