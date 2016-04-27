#ifndef UPDATE_LITE_CONSOLIDATION_H
#define UPDATE_LITE_CONSOLIDATION_H

#include <cstdint>
#include <ostream>
#include <vector>
#include <cassert>

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
        char t = RorW ? 'w' : 'r';
        return out << t << "-posts: " << io.postings << ' ' << t << "-seeks: " << io.seeks;
    }

    template<typename IO >
    double costIoInMinutes(IO io, uint64_t ioMBS, double ioSeeks, unsigned postingSzBytes) {
        const uint64_t bytesPerMs =  (ioMBS<<20)/(uint64_t(1000));
        const uint64_t ms = ioSeeks * io.seeks + //seek time
                            (io.postings * postingSzBytes)/bytesPerMs; //RW time
        return double(ms)/60000.0; //convert to minutes
    }

    typedef DiskIOCost<false> ReadIO;
    typedef DiskIOCost<true> WriteIO;

    class ConsolidationStats {
    public:
        ReadIO reads;
        WriteIO writes;
        ConsolidationStats(ReadIO rs, WriteIO ws) : reads(rs), writes(ws){}
        ConsolidationStats(size_t rd=0, size_t rs=0, size_t wd=0, size_t ws=0) :
                reads(ReadIO(rd,rs)), writes(WriteIO(wd,ws)){}
        ConsolidationStats& operator+=(const ConsolidationStats& rhs) {
            reads += rhs.reads;
            writes += rhs.writes;
            return *this;
        }
        ConsolidationStats& operator+=(WriteIO w) {
            writes += w;
            return *this;
        }

        static double costInMinutes(const ConsolidationStats& stats,
                                    uint64_t ioMBS, double ioSeeks, unsigned postingSzBytes) {
            return costIoInMinutes(stats.writes,ioMBS,ioSeeks,postingSzBytes) +
                    costIoInMinutes(stats.reads,ioMBS,ioSeeks,postingSzBytes);
        }
    };


    template<typename T>
    inline bool isComparableSize(T stackBottom, T stackTop) {
        return ((stackBottom *15) >> 4) <= stackTop;
    }

    //can't get empty stack, but can get a 1-sized
    //if one segment -- returns 0
    //if last two are not comparable, returns size-1
    //return of size-1 usually means -- write down the last one!
    //if every suffix is comparable to the one before suffix, will return 0
    template<typename T>
    unsigned offsetOfTelescopicMerge(const std::vector<T>& sizeStack) {
        //TODO: use reverse iterator?
        assert(sizeStack.size());
        auto i = sizeStack.size()-1; //point to last
        auto accumulate = sizeStack[i];
        for(; i>0; --i)
            if(! isComparableSize(sizeStack[i-1],accumulate))
                return i;
            else
                accumulate +=sizeStack[i-1];
        return 0;
    }

    ConsolidationStats consolidateSegments(std::vector<uint64_t>& segments, unsigned offset);

    inline std::ostream& operator<<(std::ostream& out, const ConsolidationStats& io) {
        return out << io.reads << ' ' << io.writes;
    }

    //this one doesn't work with <2 segments!
    template<typename IT>
    ConsolidationStats kWayConsolidate(IT begin, IT end) {
        std::vector<uint64_t > segments(begin,end);
        return consolidateSegments(segments, 0);
    }
}

#endif //UPDATE_LITE_CONSOLIDATION_H
