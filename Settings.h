#ifndef UPDATE_LITE_SETTINGS_H
#define UPDATE_LITE_SETTINGS_H

#include <cstdint>
#include <vector>

namespace IndexUpdate {
    class Settings {
    public:
        uint64_t totalExperimentPostings;
        uint64_t updateBufferPostingsLimit;
        uint64_t  updatesQuant;
        uint64_t  quieriesQuant;

        typedef std::vector<uint64_t> dataC;
        dataC tpMembers;
        dataC tpUpdates;
        dataC tpQueries;
    };
}

#endif //UPDATE_LITE_SETTINGS_H
