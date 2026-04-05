#ifndef RR_H
#define RR_H

#include <string>
#include "schedule.h"
#include "policy.h"

using namespace local;

namespace local {
    namespace policy {
        class RR : public Policy
        {
        public:
            Policy evaluate(Schedule s);

            RR(std::string name, Trace trace, int quantum) : Policy(name, trace, quantum)
            {}
        };
    }
}

#endif
