#ifndef UPDATE_LITE_CONSOLIDATION_H
#define UPDATE_LITE_CONSOLIDATION_H

#include <cstdint>
#include <ostream>

namespace IndexUpdate {
    template<bool RorW>
    struct DiskIOCost {
        uint64_t  postings;
        uint64_t  seeks;
        DiskIOCost(uint64_t posts=0, uint64_t sks=0) :
                postings(posts),seeks(sks) {}

        DiskIOCost& operator+=(const DiskIOCost& rhs) {
            postings += rhs.postings;
            seeks += rhs.seeks;
            return *this;
        }
    };

    template<bool RorW>
    inline DiskIOCost<RorW> operator+(DiskIOCost<RorW> a, DiskIOCost<RorW> b) {
        a+=b;
        return a;
    }

    template<bool RorW>
    std::ostream& operator<<(std::ostream& out, DiskIOCost<RorW> io) {
        return out << "r-posts: " << io.postings << " r-seeks: " << io.seeks;
    }

    typedef DiskIOCost<false> ReadIO;
    typedef DiskIOCost<true> WriteIO;

    class Consolidation {

    };
}

#endif //UPDATE_LITE_CONSOLIDATION_H
