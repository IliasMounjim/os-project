#ifndef POLICY_H
#define POLICY_H

#include <string>
#include <vector>
#include "schedule.h"

namespace local {
    namespace policy {
        class Event
        {
            private:
                int start;
                int end;
                int jobID;

            public:
                int getStart() { return start; }
                int getEnd() { return end; }
                int getID() { return jobID; }

                bool operator== (const Event &other) const { return (this->jobID == other.jobID) && (this->start == other.start) && (this->end == other.end); }
                bool operator!= (const Event &other) const { return !(*this == other); }
                
                Event(int start = -1, int end = -1, int id = -1)
                    : start(start)
                    , end(end)
                    , jobID(id)
                {}
        };
        
        class Trace
        {
            public:
                Schedule s;
                std::vector<Event> trace;
                void addEvent(Event e);
                Event getEvent(int i);
                Event getLastOccured(int id);
                Event getFirstOccured(int id);
                float fairnessEvent(int id);
           
                Trace()
                {}
        };
        
        class Policy 
        {
            public:
                int quantum; //for RR and Lottery
                std::string name;
                Trace trace;

                void printTraceAnalysis();

                Policy(std::string name, Trace trace, int quantum)
                    : name(name)
                    , trace(trace)
                    , quantum(quantum)
                {}
        };
    }
}

#endif
