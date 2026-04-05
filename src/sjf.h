#ifndef SJF_H
#define SJF_H

#include <string>
#include "schedule.h"
#include "policy.h"

using namespace local;

namespace local {
    namespace policy {
        class SJF : public Policy
        {
        public:
            Policy evaluate(Schedule s);

            SJF(std::string name, Trace trace, int quantum) : Policy(name, trace, quantum)
            {}
        };
    }
}

#endif
