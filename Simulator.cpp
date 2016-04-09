#include "Simulator.h"
#include "Settings.h"
#include "TermPack.h"

#include <iostream>
#include <chrono>
#include <cassert>
#include <vector>
#include <memory>

namespace IndexUpdate {

    enum Algorithm {
        NeverMerge, AlwaysMerge, LogMerge, SkiBased, Prognosticator
    };
    const char* AlgNames[] = {"Never", "Always", "Logarithmic", "SkiBased", "Prognosticator"};

    class SimulatorIMP {
        const Settings settings;

        uint64_t totalSeenPostings;
        uint64_t postingsInUpdateBuffer;
        uint64_t lastQueryAtPostings;
        unsigned  queriesStoppedAt;

        uint64_t totalQs;
        unsigned evictions;

        ReadIO totalReads;

        std::vector<TermPack> tpacks;
        std::vector<uint64_t> monolithicSegments;

    public:
        SimulatorIMP(const Settings &s);

        ~SimulatorIMP() { }

        const SimulatorIMP& execute(Algorithm alg);
        void init();
        bool finished() const;
        bool bufferFull() const;
        void handleQueries();
        void fillUpdateBuffer();
        void evictFromUpdateBuffer(Algorithm alg);
        void evictMonoliths(Algorithm alg);
        void evictTPacks(Algorithm alg);

        void report(Algorithm alg) const;
    };

    void Simulator::simulate(const Settings &settings) {
        auto start = std::chrono::system_clock::now();
        std::unique_ptr<SimulatorIMP> simulator(new SimulatorIMP(settings));
        for(auto alg : {AlwaysMerge})//, LogMerge, SkiBased, Prognosticator} )
            simulator->execute(alg).report(alg);

        auto end = std::chrono::system_clock::now();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "Done ( " << millis << "ms. )\n";
    }

    SimulatorIMP::SimulatorIMP(const Settings &s) :
            settings(s),
            totalSeenPostings(0),
            postingsInUpdateBuffer(0),
            lastQueryAtPostings(0),
            queriesStoppedAt(0),
            totalQs(0),
            evictions(0)
            {    }

    const SimulatorIMP&  SimulatorIMP::execute(Algorithm alg) {
        try {
            init();
            while (!finished()) {
                fillUpdateBuffer();
                handleQueries();
                evictFromUpdateBuffer(alg);
            }
        }
        catch (std::exception &e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
        return *this;
    }


    void SimulatorIMP::init() {
        assert(settings.updatesQuant > 9999); //no point to make it too small
        assert(settings.totalExperimentPostings * settings.quieriesQuant);
        assert(settings.updateBufferPostingsLimit > settings.updatesQuant);
        assert(settings.tpQueries.size() == settings.tpUpdates.size());
        assert(settings.tpQueries.size() == settings.tpMembers.size());

        totalSeenPostings = postingsInUpdateBuffer =
        evictions = totalQs =
        queriesStoppedAt = lastQueryAtPostings = 0;

        totalReads = ReadIO();
        auto updates = settings.tpUpdates.begin();
        auto queries = settings.tpQueries.begin();
        auto members = settings.tpMembers.begin();

        //init term packs
        tpacks.clear();
        for(unsigned i=0; updates != settings.tpUpdates.end();
                ++updates, ++queries, ++members, ++i) {
            tpacks.emplace_back(TermPack(i,*members, *updates, *queries));
        }
        TermPack::normalizeUpdates(tpacks);
    }

    void SimulatorIMP::report(Algorithm alg) const{
        std::cout <<
                AlgNames[alg] << " ==> " <<
                " Total-seen-postings: " << totalSeenPostings <<
                " Total-queries: " << totalQs <<
                " Evictions: " << evictions <<
                " Total-reads: " << totalReads <<
                std::endl;
    }

    void SimulatorIMP::handleQueries() {
        auto total = totalSeenPostings+postingsInUpdateBuffer;
        if(total >= lastQueryAtPostings + settings.updatesQuant) {
            auto totalNew = total - lastQueryAtPostings;
            auto carry = totalNew % settings.updatesQuant;
            assert(total > carry);
            lastQueryAtPostings = total - carry;

            //ask your quant of queries
            auto quant = settings.quieriesQuant * (totalNew / settings.updatesQuant);
            assert(quant);
            totalQs += quant;
            //currently RoundRobin -- replace with a discrete distribution
            for(; quant != 0; --quant, queriesStoppedAt = (queriesStoppedAt+1)%tpacks.size()) {
                   totalReads += tpacks[queriesStoppedAt].query();
            }
        }
    }

    void SimulatorIMP::fillUpdateBuffer() {
        while (!bufferFull()) {
            for(auto& tp : tpacks) //round robin
                postingsInUpdateBuffer += tp.addUBPostings();
        }
    }

    void SimulatorIMP::evictMonoliths(Algorithm alg) {
        for(auto& tp : tpacks) {
            auto newPostings = tp.evictAll();
            auto& segments = tp.unsafeGetSegments();
            segments.clear();
            segments.push_back(newPostings);
        }
        //consolidation cost is calc. here...

        totalSeenPostings += postingsInUpdateBuffer;
        postingsInUpdateBuffer = 0;
    }

    void SimulatorIMP::evictTPacks(Algorithm alg) {
        totalSeenPostings += postingsInUpdateBuffer;
        postingsInUpdateBuffer = 0;
    }


    void SimulatorIMP::evictFromUpdateBuffer(Algorithm alg) {
        ++evictions;

        if(alg != SkiBased && alg != Prognosticator)
            evictMonoliths(alg);
        else
            evictTPacks(alg);
    }

    bool SimulatorIMP::bufferFull() const {
        return postingsInUpdateBuffer >= settings.updateBufferPostingsLimit;
    }

    bool SimulatorIMP::finished() const {
        return totalSeenPostings >= settings.totalExperimentPostings;
    }

}