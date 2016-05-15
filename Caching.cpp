#include <iomanip>
#include <iostream>
#include <algorithm>
#include <cassert>
#include "Caching.h"

namespace Caching {



//==============================================================================================
    class BaseCache::BaseCacheIMPL {
    public:
        std::unordered_map<term_t, Term*> lookupTable;
        Term *lookup(term_t term) {
            auto it = lookupTable.find(term);
            return (it == lookupTable.end()) ?
                                nullptr :  it->second;
        }

        Term* placeNew(term_t term, size_t length) {
            Term *t = new Term();
            t->term = term;
            t->length = length;
            lookupTable[term] = t;
            return t;
        }

        inline void evict(term_t term) {
            auto it = lookupTable.find(term);
            assert(it != lookupTable.end());
            if(it->second->hitCount < 3) { //higher values => smaller lookup table, closer to vanilla
                delete it->second;
                lookupTable.erase(it);
            }
            else
                it->second->length = 0;
        }

        size_t tableSz() const { return lookupTable.size(); }
        size_t size() const { return std::count_if(lookupTable.begin(),lookupTable.end(),
                                                   [](std::unordered_map<term_t, Term*>::const_reference val)
                                                   { return val.second->length > 0;}); }
    };
//==============================================================================================
    size_t BaseCache::size() const { return baseimpl->size(); }
    Term* BaseCache::lookup(term_t term) const {return baseimpl->lookup(term); }
    Term* BaseCache::placeNew(term_t term, size_t length) { return baseimpl->placeNew(term,length); }
    void BaseCache::evict(term_t t) { return baseimpl->evict(t);}

    BaseCache::BaseCache() :
            cacheHits(0),cachePostingsServed(0),
            cachePostingsMissed(0),cacheRejected(0),
            baseimpl(new BaseCacheIMPL()){}
    BaseCache::~BaseCache() { delete baseimpl; }

    void BaseCache::setMaxPostings(size_t maxP) {
        if(maxPostings)
            throw("can't resize cache in this implementation");
        maxPostings = maxP;
    }

    bool BaseCache::visit(unsigned term, size_t length) {
        auto tptr = BaseCache::lookup(term);
        if(tptr && tptr->length) { //hit
            ++cacheHits;
            cachePostingsServed += length;
            hit(tptr, length);
            return true;
        }
        else if(!tptr) { //full miss
            if(maxPostings <= length)
                ++cacheRejected;
            else
                miss(term, length);
            cachePostingsMissed += length;
            return false;
        }
        //hit of evicted
        hit(tptr, length);
        cachePostingsMissed += length;
        return false;
    }

    void BaseCache::report(std::ostream& out, unsigned totalQs)const {
        //size_t acc = 0; for(auto t: baseimpl->lookupTable ) acc += t.second.length; //expect_eq getTotalP()
        out << name()
            << " hits: " << std::setw(9) << cacheHits
            << " hit-pct: " << std::setw(5) << std::fixed << std::setprecision(2)  << double(cacheHits)/double(totalQs) * 100.0
            << " size: " << std::setw(14) << getTotalP()
            << " members: " << std::setw(4) << size() << '(' << baseimpl->tableSz() << ')'
            << " rejects: " << std::setw(6) << cacheRejected
            << " postings-served: " << std::setw(14) << cachePostingsServed
            << " postings-missed: " << std::setw(14) << cachePostingsMissed
            << " srv-pct: " << std::fixed <<  std::setprecision(2) << double(cachePostingsServed)/double(cachePostingsServed+cachePostingsMissed) * 100.0
            << std::endl;
    }

}