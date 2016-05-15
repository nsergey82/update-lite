#ifndef CACHING_CACHING_H
#define CACHING_CACHING_H


#include <unordered_map>
#include <vector>
#include <list>
#include <ostream>

namespace  Caching {
    typedef unsigned term_t;

    class Term {
    public:
        term_t term;
        unsigned hitCount; //how many times was hit since introduction to cache
        size_t length;
        uint64_t L;

        Term() : hitCount(1) { }

        inline uint64_t cost() const { return length * size_t(hitCount); }
    };

    struct ReverseByFreqComparator {
        inline bool operator()(const Term* a, const Term* b) const { return a->hitCount == b->hitCount ? a->term < b->term : a->hitCount < b->hitCount;}
    };

    struct ReverseByCost1Comparator {
        inline bool operator()(const Term* a, const Term* b) const { return a->cost() == b->cost()  ? a->term < b->term : a->cost() < b->cost(); }
    };

    struct ReverseByLComparator {
        inline bool operator()(const Term* a, const Term* b) const { return a->L == b->L  ? a->term < b->term : a->L < b->L; }
    };

    class CacheInterface {
    public:
        virtual ~CacheInterface() { }

        //length is the up-to-date length of the term on disk (could be smaller if deletes)
        virtual bool visit(term_t term, size_t length) = 0;
    };

    class BaseCache : public CacheInterface {
    public:
        size_t cacheHits;
        size_t cachePostingsServed;
        size_t cachePostingsMissed;
        size_t cacheRejected;
        size_t maxPostings;

        BaseCache();

        virtual ~BaseCache();

        virtual void setMaxPostings(size_t maxP);

        virtual size_t getTotalP() const = 0;

        bool visit(unsigned term, size_t length);

        size_t size() const; //return the count of cached terms (size of lookup)

        virtual std::string name() const = 0;
        void report(std::ostream& out, unsigned totalQs)const;
    protected:
        virtual void miss(term_t term, size_t length) = 0;

        virtual void hit(Term *tptr, size_t length) = 0;

        Term *lookup(term_t term) const;

        Term *placeNew(term_t term, size_t length);

        //void evictMany(const std::vector<term_t> &victims);

        void evict(term_t t);

        class BaseCacheIMPL;

        BaseCacheIMPL *baseimpl;
    };
}

#endif //CACHING_CACHING_H
