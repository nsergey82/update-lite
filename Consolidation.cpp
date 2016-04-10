#include <algorithm>
#include "Consolidation.h"

namespace IndexUpdate {
//does int. division and taking correct upper
    inline size_t _rfdiv(size_t a, size_t share) {
        return /*(a<share) ? 1 : */(a / share + size_t(a % share != 0));
    }

    class KWaySegmentConsolidator {
        std::vector<size_t> &segHeap;
        const size_t memSizeInPostings;
        size_t lastSize;
        ConsolidationStats cost;

        //while we can put many into buffer completely
        bool roundKWay() {
            auto smallest1 = pop();
            auto smallest2 = pop();

            size_t reads = smallest1 + smallest2;
            size_t writes = reads;
            unsigned totalReadSeeks = 2;

            //if one of the smallest is in mem
            if (lastSize && (lastSize == smallest1 || lastSize == smallest2)) {
                --totalReadSeeks;
                reads -= lastSize;
                lastSize = 0;
            }

            //can't handle even the two smallest
            if (reads > memSizeInPostings / 2) { //roll back
                push(smallest2);
                push(smallest1);
                if (writes != reads) //it is possible one was in-mem
                    lastSize = writes - reads;
                return false;
            }

            while (!segHeap.empty()) {
                size_t seg = pop();
                if (seg == lastSize) {
                    lastSize = 0;
                }
                else {
                    if (reads + seg > memSizeInPostings / 2) { //too much
                        push(seg);
                        break;
                    }
                    reads += seg;
                    ++totalReadSeeks;
                }
                writes += seg;
            }

            push(writes);
            //S,R,W
            cost += ConsolidationStats(reads, totalReadSeeks, writes, 1);
            return true;
        }

        size_t pop() {
            std::pop_heap(segHeap.begin(), segHeap.end(), std::greater<size_t>());
            auto smallest = segHeap.back();
            segHeap.pop_back();
            return smallest;
        }

        void push(size_t s) {
            segHeap.push_back(s);
            std::push_heap(segHeap.begin(), segHeap.end(), std::greater<size_t>());
        }

        inline size_t writePShare() const { return (memSizeInPostings / 2); }

        void round2Way() {
            const auto writeShare = writePShare();
            const auto readShare = (writeShare / 2);

            auto smallest1 = pop();
            auto smallest2 = pop();

            size_t readSeeks = 0;
            readSeeks += _rfdiv(smallest1, readShare);
            readSeeks += _rfdiv(smallest2, readShare);
            size_t readSeg = smallest1 + smallest2;
            size_t writeSeg = smallest1 + smallest2;
            size_t writeSeeks = _rfdiv(writeSeg, writeShare);

            //fix seeks and reads if in mem
            if (lastSize && (lastSize == smallest1 || lastSize == smallest2)) {
                readSeg -= lastSize;
                readSeeks -= _rfdiv(lastSize, readShare);
                lastSize = 0; //this can happen only once per consolidation!
            }

            push(writeSeg);
            cost += ConsolidationStats(readSeg,readSeeks, writeSeg, writeSeeks);
        }

    public:

        KWaySegmentConsolidator(std::vector<size_t> &segments,
                                size_t lastInMem = 1 /*0 or 1*/, size_t memBufferPostings = 1ULL<<26)
                : segHeap(segments), memSizeInPostings(memBufferPostings) {

            assert(lastInMem < 2);
            lastSize = segHeap.back() * lastInMem;
            std::make_heap(segHeap.begin(), segHeap.end(), std::greater<size_t>());
        }

        ConsolidationStats operator()() {
            cost = ConsolidationStats();
            //attempt k-way consolidation of all the segments that fit completely into mem
            while (segHeap.size() > 1 && roundKWay())
                continue;
            while (segHeap.size() > 1) //always consolidate two smallest
                round2Way();

            return cost;
        }
    };

    ConsolidationStats consolidateSegments(std::vector<uint64_t>& segments, unsigned offset) {
        assert(segments.size()>1);
        std::vector<uint64_t> consolidants(segments.begin() + offset, segments.end());

        auto writtenPostings = std::accumulate(segments.begin()+offset,segments.end(),0ull);
        segments.erase(segments.begin()+offset,segments.end());
        segments.push_back(writtenPostings);

        return KWaySegmentConsolidator(consolidants)();
    }

}