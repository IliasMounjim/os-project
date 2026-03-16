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
                
                Event(int start, int end, int id)
                    : start(start)
                    , end(end)
                    , jobID(id)
                {}
        };
        
        class Trace
        {
            public:
                std::vector<Event> trace;
                void addEvent(Trace trace, Event e);
            
                Trace()
                {}
        };
        
        class Policy 
        {
            public:
                int quantum; //for RR and Lottery
                std::string name;
                Trace trace;

                Policy evaluate(Policy self, Schedule s);
                void printTraceAnalysis(Policy self);

                Policy(std::string name, Trace trace, int quantum)
                    : name(name)
                    , trace(trace)
                    , quantum(quantum)
                {}
        };
    }
}
