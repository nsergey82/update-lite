#include "TermPack.h"

#include <algorithm>

namespace IndexUpdate {

    bool operator<(const TermPack& a,const TermPack& b) {
        return a.updates() < b.updates();
    }

    void TermPack::normalizeUpdates(std::vector<TermPack> &tpacks, double reduceTo) {
        assert(!tpacks.empty());
        auto smallestE = std::min_element(tpacks.begin(),tpacks.end());
        double smallest = smallestE->updates();
        assert(smallest > reduceTo);
        double div = smallest / reduceTo;
        for(auto& tp : tpacks)
            tp.tpNormalizedUpdates = round(double(tp.tpEpochUpdates)/div);
    }
}