#ifndef PRIORITY_H
#define PRIORITY_H

#include <string>
#include "schedule.h"
#include "policy.h"

using namespace local;

namespace local {
    namespace policy {
        class Priority: public Policy
        {
            public:
                Policy evaluate(Schedule s);

                Priority(std::string name, Trace trace, int quantum) : Policy(name, trace, quantum)
                {}
        };
    }
}

#endif