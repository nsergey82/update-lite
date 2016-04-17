#ifndef UPDATE_LITE_SETTINGS_H
#define UPDATE_LITE_SETTINGS_H

#include <cstdint>
#include <vector>
#include <cstddef>
#include <functional>

namespace IndexUpdate {
    enum Algorithm {
        NeverMerge, AlwaysMerge, LogMerge, SkiBased, Prognosticator
    };
    enum DiskType { HD, SSD};

    class Settings {
    public:
        DiskType diskType;
        unsigned ioMBS; //how many MB per second we can read/write
        double ioSeek; //the time to make an average seek (random access latency)
        unsigned szOfPostingBytes; //we use fixed size of postings (in bytes).

        //how many postings we are going to accomodate
        uint64_t totalExperimentPostings;
        //the size of update buffer in postings
        uint64_t updateBufferPostingsLimit;
        //the two quants represent the update-to-query ratio
        uint64_t  updatesQuant; //usually one million
        uint64_t  quieriesQuant;

        typedef std::vector<uint64_t> dataC;
        //TPack related data (based on our training set)
        dataC tpMembers;
        dataC tpUpdates;
        dataC tpQueries;

        static size_t hash(const Settings& s);
        static const std::string& name(Algorithm alg);
    };

    bool operator==(const Settings& lhs, const Settings& rhs) ;
}

namespace std {
    template<> struct hash<IndexUpdate::Settings> {
        size_t operator()(const IndexUpdate::Settings &settings) const {
            return IndexUpdate::Settings::hash(settings);
        }
    };

}

#endif //UPDATE_LITE_SETTINGS_H
