#ifndef FCFS_H
#define FCFS_H

#include <string>
#include "schedule.h"
#include "policy.h"

using namespace local;

namespace local {
    namespace policy {
        class FCFS : public Policy
        {
            public:
                Policy evaluate(Schedule s);

                FCFS(std::string name, Trace trace, int quantum) : Policy(name, trace, quantum)
                {}
        };
    }
}

#endif
