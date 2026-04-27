#ifndef HYBRID_H
#define HYBRID_H

#include <string>
#include "schedule.h"
#include "policy.h"

using namespace local;

namespace local {
    namespace policy {
        class Hybrid : public Policy
        {
        public:
            Policy evaluate(Schedule s);

            Hybrid(std::string name, Trace trace, int quantum) : Policy(name, trace, quantum)
            {}
        };
    }
}

#endif
