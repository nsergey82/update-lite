#ifndef CACHING_LANDLORD_H
#define CACHING_LANDLORD_H

#include "Caching.h"

#include <set>


namespace  Caching {
    typedef std::multiset<Term*, ReverseByLComparator> MinHeapByL;

    class Landlord : public BaseCache {
        size_t totalPostings;
        uint64_t accumulator;
        MinHeapByL heap;
    public:
        explicit Landlord(size_t maxPstings=0);
        virtual std::string name() const { return "landlord1"; }
        virtual size_t getTotalP() const { return totalPostings; }
    protected:
        virtual void miss(term_t term, size_t length);
        virtual void hit(Term* tptr, size_t length);
    };
}
#endif //CACHING_LANDLORD_H
