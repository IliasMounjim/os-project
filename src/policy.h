#ifndef POLICY_H
#define POLICY_H

#include "src/schedule.h"
#include <string>
#include <vector>

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
                
                Event(int start, int end, int id)
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
