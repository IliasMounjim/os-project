// ========================================================================
// hybrid.h
// ------------------------------------------------------------------------
// Description: Header for the Hybrid policy class. Same shape as the
//   other policy headers (FCFS, SJF, RR, etc.). The actual decision
//   logic lives in hybrid.cpp.
// ------------------------------------------------------------------------
// Author: Illiasse Mounjim
// Version: 2026.04.27
// ========================================================================

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
