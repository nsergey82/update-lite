#ifndef UPDATE_LITE_TERMPACK_H
#define UPDATE_LITE_TERMPACK_H

#include <cstdint>
#include <vector>
#include <cassert>

#include "Consolidation.h"

namespace IndexUpdate {
    class TermPack {
        unsigned tpId;
        unsigned tpMembersCount;

        uint64_t tpEpochUpdates;
        uint64_t tpEpochQueries;
        uint64_t tpNormalizedUpdates;

        uint64_t tpUBPostings;
        uint64_t tpEvictedPostings;

        uint64_t tpExtraSeeks;
        uint64_t tpTokens;

        std::vector<uint64_t> tpSegments;

    public:
        TermPack(unsigned id=0, unsigned members=0, uint64_t upd=0, uint64_t qs=0 )
                : tpId(id),tpMembersCount(members),
                  tpEpochUpdates(upd),tpEpochQueries(qs),
                  tpUBPostings(0), tpEvictedPostings(0),
                  tpExtraSeeks(0),tpTokens(0.0)
        {}

        uint64_t addUBPostings() {
            assert(tpNormalizedUpdates);
            tpUBPostings += tpNormalizedUpdates;
            return tpNormalizedUpdates;
        }

        uint64_t evictAll() {
            auto evicted = tpUBPostings;
            tpEvictedPostings += evicted;
            tpUBPostings = 0;
            return evicted;
        }

        std::vector<uint64_t>& unsafeGetSegments() { return tpSegments; }
        const std::vector<uint64_t>& segments() const { return tpSegments; }

        uint64_t updates() const { return tpEpochUpdates; }

        uint64_t extraSeeks() const { return  tpExtraSeeks; }
        double convertSeeksToTokens(double tokens) {
            tpExtraSeeks = 0;
            tpTokens += tokens;
            return tpTokens;
        }

        ReadIO query() {
            assert(!tpSegments.empty());
            tpExtraSeeks += tpSegments.size()-1;
            return ReadIO(tpEvictedPostings/tpMembersCount, tpSegments.size());
        };

        static void normalizeUpdates(std::vector<TermPack> &tpacks, double reduceTo = 1<<14);
    };

    bool operator<(const TermPack& a,const TermPack& b);
}


#endif //UPDATE_LITE_TERMPACK_H
