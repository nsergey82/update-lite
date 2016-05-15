#include "Landlord.h"
#include <cassert>
#include <cmath>

namespace  Caching {

    inline uint64_t LFromLength(uint64_t len) { return len;}

    Landlord::Landlord(size_t maxPstings) : totalPostings(0), accumulator(0) {
        maxPostings = maxPstings;
    }

    void Landlord::miss(term_t term, size_t length) {
        while (totalPostings > maxPostings) { //remove overflows!
            assert(!heap.empty());
            auto it = heap.begin();
            Term *tptr = *it;
            assert(totalPostings >= tptr->length);
            totalPostings -= tptr->length;
            BaseCache::evict(tptr->term);
            heap.erase(it);
        }
        while (totalPostings + length > maxPostings) { //evict to accommodate with bound size policy
            assert(!heap.empty());
            auto it = heap.begin();
            Term *tptr = *it;
            if(tptr->L > accumulator)
                return; //don't add it!

            assert(totalPostings >= tptr->length);
            totalPostings -= tptr->length;
            BaseCache::evict(tptr->term);
            heap.erase(it);
        }
        Term *tptr = placeNew(term, length);
        accumulator +=  LFromLength(length);
        tptr->L = accumulator;
        totalPostings += length;
        heap.insert(tptr);
    }
#define DODGY_HIT_MODE
    void Landlord::hit(Term *tptr, size_t newLength) {
        heap.erase(tptr); //erase first, since
        ++(tptr->hitCount); //could be violating the map now!
        uint64_t mult = 1; // tptr->hitCount
        tptr->L = accumulator + (LFromLength(newLength) * mult); //reset L
        if(tptr->length != newLength) { //cache data not coherent
#ifdef DODGY_HIT_MODE
            assert(totalPostings >= tptr->length);
            totalPostings -= tptr->length;
            totalPostings += newLength;
            tptr->length = newLength;
#endif
        }
        heap.insert(tptr);
    }

}
