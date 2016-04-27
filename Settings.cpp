#include "Settings.h"

#include <functional>

namespace IndexUpdate {
    template<typename T>
    unsigned long hashMe(T val) { return std::hash<T>()(val); }

    size_t Settings::hash(const Settings& s) {
        return
            hashMe(s.ioMBS) ^
            hashMe(s.ioSeek) ^
            hashMe(s.szOfPostingBytes) ^
            hashMe(s.totalExperimentPostings) ^
            hashMe(s.updateBufferPostingsLimit) ^
            hashMe(s.updatesQuant) ^
            hashMe(s.quieriesQuant) ^ hashMe(s.percentsUBLeft);
    }

    bool operator==(const Settings& lhs, const Settings& rhs) {
       return
            lhs.ioMBS == rhs.ioMBS &&
            lhs.ioSeek == rhs.ioSeek &&
            lhs.szOfPostingBytes == rhs.szOfPostingBytes &&
            lhs.totalExperimentPostings == rhs.totalExperimentPostings &&
            lhs.updateBufferPostingsLimit == rhs.updateBufferPostingsLimit &&
            lhs.updatesQuant == rhs.updatesQuant &&
            lhs.quieriesQuant == rhs.quieriesQuant &&
            lhs.percentsUBLeft == rhs.percentsUBLeft;
    }

    const std::string& Settings::name(Algorithm alg) {
        static const std::string names[] = {"Never-Merge",
                                            "AlwaysMerge",
                                            "LogrthMerge",
                                            "SkiBsdMerge",
                                            "Prgnstcator"};
        return names[alg];
    }

}