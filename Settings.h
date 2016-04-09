//
// Created by sergeyn on 09/04/16.
//

#ifndef UPDATE_LITE_SETTINGS_H
#define UPDATE_LITE_SETTINGS_H

#include <cstdint>

namespace IndexUpdate {
    class Settings {
    public:
        uint64_t totalExperimentPostings;
        uint64_t updateBufferPostingsLimit;
        uint64_t  updatesQuant;
        uint64_t  quieriesQuant;
    };
}

#endif //UPDATE_LITE_SETTINGS_H
