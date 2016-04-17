#ifndef UPDATE_LITE_SIMULATOR_H
#define UPDATE_LITE_SIMULATOR_H

#include "Settings.h"

namespace IndexUpdate {

    namespace Simulator {
        std::vector<std::string> simulate(const std::vector<Algorithm>& algs, const Settings &);
    }
}

#endif //UPDATE_LITE_SIMULATOR_H
