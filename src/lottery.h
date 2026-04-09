#ifndef LOTTERY_H
#define LOTTERY_H

#include <string>
#include "schedule.h"
#include "policy.h"

using namespace local;

namespace local {
    namespace policy {
        class LOTTERY : public Policy
        {
            public:
                Policy evaluate(Schedule s);

                LOTTERY(std::string name, Trace trace, int quantum) : Policy(name, trace, quantum)
                {}
        };
    }
}

#endif
