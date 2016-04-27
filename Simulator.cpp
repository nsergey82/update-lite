#include "Simulator.h"
#include "Settings.h"
#include "TermPack.h"

#include <iostream>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace IndexUpdate {

    void auxRebuildCPrice(std::vector<double> &consolidationPriceVector, const std::vector<size_t>& sizeStack,
                          const Settings &settings);

    class SimulatorIMP {
        const Settings settings;

        uint64_t totalSeenPostings;
        uint64_t postingsInUpdateBuffer;
        uint64_t lastQueryAtPostings;
        unsigned  queriesStoppedAt;

        uint64_t totalQs;
        unsigned evictions;

        ReadIO totalQueryReads;

        ConsolidationStats merges;
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

        ConsolidationStats consolidateTPSki(TermPack& tp) {
            auto segments = tp.segments();
            ConsolidationStats nil;
            if(segments.size()<2) {
                nil += WriteIO(segments.back(), 0); //0 since we write all non-consolidants together
                return nil;
            }

            double tokens = tp.convertSeeksToTokens( //exchange seeks for tokens
                                 costIoInMinutes(ReadIO(0,tp.extraSeeks()),
                                    settings.ioMBS, settings.ioSeek, settings.szOfPostingBytes));

            std::vector<double> consolidationPriceVector;
            auxRebuildCPrice(consolidationPriceVector, segments, settings);

            auto i = int(consolidationPriceVector.size()-1);
            while(i>=0 && tokens >= consolidationPriceVector[size_t(i)])
                --i;
            unsigned offset = i+1;

            assert(offset<=segments.size());
            if(offset<segments.size()-1) {
                auto cons = consolidateSegments(tp.unsafeGetSegments(), offset);
                tp.reduceTokens(ConsolidationStats::costInMinutes(cons,
                                                settings.ioMBS, settings.ioSeek, settings.szOfPostingBytes));
                return cons;
            }

            nil += WriteIO(segments.back(),0); //0 since we write all non-consolidants together
            return nil;
        }

        ConsolidationStats consolidateTPStatic(TermPack& tp) {
            auto &segments = tp.unsafeGetSegments();
            auto offset = offsetOfTelescopicMerge(segments);
            assert(offset<=monolithicSegments.size());
            if(offset<monolithicSegments.size()-1)
                return consolidateSegments(segments, offset);

            ConsolidationStats nil;
            nil += WriteIO(segments.back(),1);
            return nil;
        }

        std::string report(Algorithm alg) const;
    };

    std::vector<std::string> Simulator::simulate(const std::vector<Algorithm>& algs, const Settings &settings) {
        //auto start = std::chrono::system_clock::now();
        std::vector<std::string> reports;
        for(auto alg : algs)
            reports.emplace_back(
                    SimulatorIMP(settings).execute(alg).report(alg));

        //auto end = std::chrono::system_clock::now();
        //std::cerr << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms\n";
        return reports;
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

        totalQueryReads = ReadIO();
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

    std::string SimulatorIMP::report(Algorithm alg) const{
        auto totalQueryTime = costIoInMinutes(totalQueryReads,
                                              settings.ioMBS, settings.ioSeek, settings.szOfPostingBytes);
        auto mergeTimes = ConsolidationStats::costInMinutes(merges,
                                              settings.ioMBS, settings.ioSeek, settings.szOfPostingBytes);

        std::stringstream strstr;
        strstr <<
                 Settings::name(alg) << " " << (settings.diskType==HD?"HD":"SSD") <<
                " Evictions: " << std::setw(5) << evictions <<
                " Total-seen-postings: " << totalSeenPostings <<
                " Total-queries: " << totalQs <<
                " Query-reads: " << totalQueryReads <<
                " Consolidation: " << merges <<
                " Total-query-minutes: " << totalQueryTime <<
                " Total-merge-minutes: " << mergeTimes <<
                " Sum-All: " << totalQueryTime+mergeTimes <<
                std::endl;
        return strstr.str();
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
            //currently RoundRobin -- may replace with a discrete distribution
            for(; quant != 0; --quant, queriesStoppedAt = (queriesStoppedAt+1)%tpacks.size()) {
                   totalQueryReads += tpacks[queriesStoppedAt].query();
            }
        }
    }

    void SimulatorIMP::fillUpdateBuffer() {
        while (!bufferFull()) {
            for(auto& tp : tpacks) //round robin
                postingsInUpdateBuffer += tp.addUBPostings();
            if(finished())
                break;
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
        monolithicSegments.push_back(postingsInUpdateBuffer);
        totalSeenPostings += postingsInUpdateBuffer;
        postingsInUpdateBuffer = 0;

        auto offset = (LogMerge == alg) ? offsetOfTelescopicMerge(monolithicSegments) :
                                (monolithicSegments.size() > 1 ? 0 : 1);

        //override for NeverMerge
        if(NeverMerge == alg) offset = monolithicSegments.size()-1;
        assert(offset<=monolithicSegments.size());

        if(offset<monolithicSegments.size()-1)
            merges += consolidateSegments(monolithicSegments, offset);
        else
            merges += WriteIO(monolithicSegments.back(),1);

        //fix segment sizes for tpacks (this how we know during queries how many seeks to make)
        unsigned currentSzAll = monolithicSegments.size();
        for(auto& tp : tpacks)
            tp.unsafeGetSegments().resize(currentSzAll);
    }

    void SimulatorIMP::evictTPacks(Algorithm alg) {
        uint64_t desiredCapacity = settings.percentsUBLeft * settings.updateBufferPostingsLimit / 100;
        //we evict castes with larger ID first
        for(auto it = tpacks.rbegin(); postingsInUpdateBuffer > desiredCapacity && it !=tpacks.rend(); ++it)   {
            TermPack& tp = *it;
            auto newPostings = tp.evictAll();
            tp.unsafeGetSegments().push_back(newPostings);

            totalSeenPostings += newPostings;
            postingsInUpdateBuffer -= newPostings;

            merges += consolidateTPSki(tp); //consolidateTPStatic(tp);
        }
        assert(postingsInUpdateBuffer <= desiredCapacity);
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
        return totalSeenPostings + postingsInUpdateBuffer >= settings.totalExperimentPostings;
    }

    void auxRebuildCPrice(std::vector<double> &consolidationPriceVector, const std::vector<size_t>& sizeStack,
                          const Settings &settings) {
        const int sz = sizeStack.size();
        assert(sz >= 2);

        consolidationPriceVector.clear();
        consolidationPriceVector.reserve(sz);
        for(auto it = sizeStack.begin(); it != sizeStack.end()-1; ++it) {
            auto cons = kWayConsolidate(it,sizeStack.end());
            consolidationPriceVector.push_back(
                    ConsolidationStats::costInMinutes(cons, settings.ioMBS, settings.ioSeek, settings.szOfPostingBytes));
        }
        consolidationPriceVector.push_back( //no real consolidation - just a write-back of the last one
                ConsolidationStats::costInMinutes(ConsolidationStats(0,0,sizeStack.back(),1), settings.ioMBS, settings.ioSeek, settings.szOfPostingBytes));
    }

}