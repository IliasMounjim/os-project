#ifndef SRTF_H
#define SRTF_H

#include <string>
#include "schedule.h"
#include "policy.h"

using namespace local;

namespace local {
    namespace policy {
        class SRTF : public Policy
        {
            public:
                Policy evaluate(Schedule s);

                SRTF(std::string name, Trace trace, int quantum) : Policy(name, trace, quantum)
                {}
        };
    }
}

#endif