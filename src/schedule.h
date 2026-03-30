#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <string>
#include <vector>

namespace local {
    class Job    
    {
        private:
            bool started = false; //has job started
            int id;        //job id
            int start;     //when job start
            int percentIO; //percent chance of i/o
            int ioEnd;     //when does our io end
            int arrival;   //arrival time
            int lengthLeft;    //job length
            int status;    //job status, 1 is done
    
        public:
            Job(int id, int pio, int arrival, int lengthLeft, int status)
                : id(id)
                , percentIO(pio)
                , arrival(arrival)
                , lengthLeft(lengthLeft)
                , status(status)
            {}

            std::string jobString() { return "ID: " + std::to_string(id) + "\n" + "Arrival time: " + std::to_string(arrival) + "\n" + "Job lengthLeft: " + std::to_string(lengthLeft); }
            int decrementLength() { lengthLeft--; return lengthLeft; }
            bool setStarted(bool b) { started = b; return started; }
            int setStatus(int s) { status = s; return status; }
            int setStart(int s) { start = s; return start; }
            int setIOEnd(int e) { ioEnd = e; return ioEnd; }
            bool getStarted() { return started; }
            int getID() { return id; }
            int getStart() { return start; }
            int getPercentIO() { return percentIO; }
            int getIOEnd() { return ioEnd; }
            int getArrival() { return arrival; }
            int getLength() { return lengthLeft; }
            int getStatus() { return status; }
    };
    
    class Schedule
    {
        public:
            std::vector<Job> schedule;
            
            void printSchedule();
            void fromInput(int lengthLeft, int number, int percentIO);
            int fromJson(std::string filepath, int percentIO);

            Schedule()
            {}

            Schedule(const Schedule &s)
                : schedule(s.schedule)
            {}
    
            Schedule(int lengthLeft, int number, int percentIO)
            {
                fromInput(lengthLeft, number, percentIO);
            }

            Schedule(std::string filepath, int percentIO)
            {
                fromJson(filepath, percentIO);
            }
    };
}

#endif
