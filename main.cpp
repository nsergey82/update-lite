#include <algorithm>
#include <iostream>
#include <future>
#include <cassert>

#include "Simulator.h"


using namespace IndexUpdate;

IndexUpdate::Settings setup(IndexUpdate::DiskType disk = IndexUpdate::HD, unsigned queriesQuant = 64 );

void experiment(IndexUpdate::DiskType disk, unsigned queries);
void findOptimal(IndexUpdate::DiskType disk, unsigned queries);

uint64_t globalOpts[16] = {0};
enum names {
    gTotalMPostings,
    gQRate
};

int main(int argc, char** argv) {
    std::cout.imbue(std::locale(""));
    if(argc >= 2) {
        globalOpts[gQRate] = atoi(argv[1]);
        globalOpts[gTotalMPostings] = (argc >= 3) ? 1000ull*1000ull*atoi(argv[2]) :  64ull*1000*1000*1000;
        for (auto queries : {globalOpts[gQRate]}) {
            std::cout << "===== > " << queries << " HD...\n";
            experiment(HD, queries);
        }
        for (auto queries : {globalOpts[gQRate]}) {
            std::cout << "===== > " << queries << " SSD...\n";
            experiment(SSD, queries);
        }
    }
    else {
        std::cout << "usage: " << argv[0] << " query-rate(>=1)\n";
//        assert(argc==3);
//        auto queries = atoi(argv[1]);
//        std::string disk(argv[2]);
//        assert(disk == "HD" || disk == "SSD");
//        IndexUpdate::DiskType diskT = disk == "HD" ? HD : SSD;
//        findOptimal(diskT,queries);
    }
    return 0;
}

void experiment(IndexUpdate::DiskType disk, unsigned queries){
    Settings settings = setup(disk,queries);

    std::vector<Algorithm> ski {SkiBased};
    std::vector<Algorithm> log { LogMerge};
    std::vector<Algorithm> alw { AlwaysMerge};

    typedef std::vector<std::string> ReportT;
    std::vector<std::future<ReportT>> futuresVec;

    std::vector<std::string> reports;
    bool isAsync = true;
    for(auto percents : {98,99} ) { //larger percents ==> larger UB ==> less evictions!
        settings.flags[0] = percents;
        const auto ubsz = (1ull << 32);
        settings.updateBufferPostingsLimit = (ubsz * percents) / 100;
        settings.cacheSizePostings = ubsz - settings.updateBufferPostingsLimit;

        if(isAsync) {
            settings.flags[1] = 0;
            futuresVec.emplace_back(
                    std::async(std::launch::async, Simulator::simulate,
                               std::cref(log), settings));
            futuresVec.emplace_back(
                    std::async(std::launch::async, Simulator::simulate,
                               std::cref(alw), settings));

            for(auto reduceTo : {25,90}) {
                settings.percentsUBLeft = reduceTo;
                settings.flags[1] = reduceTo;
                futuresVec.emplace_back(
                        std::async(std::launch::async, Simulator::simulate,
                                   std::cref(ski), settings));
            }

        }
        else {
            auto v = Simulator::simulate({SkiBased,LogMerge},settings);
            reports.insert(reports.end(),v.begin(),v.end());
        }
    }


    for(auto& f : futuresVec) {
        auto v = f.get();
        reports.insert(reports.end(),v.begin(),v.end());
    }

    std::sort(reports.rbegin(), reports.rend());
    for(const auto& r : reports)
        std::cout << r;
}


IndexUpdate::Settings setup(IndexUpdate::DiskType disk, unsigned queriesQuant ) {
    IndexUpdate::Settings sets;
    sets.diskType = disk;
    if(IndexUpdate::HD == disk) {
        sets.ioMBS = 150; //how many MB per second we can read/write
        sets.ioSeek = 7; //the time to make an average seek (random access latency)
    }
    else if (IndexUpdate::SSD == disk){ //from Samsung
        sets.ioMBS = 500; //how many MB per second we can read/write
        sets.ioSeek = 0.0625; //the time to make an average seek (random access latency)
    }
    sets.quieriesQuant = queriesQuant;

    sets.szOfPostingBytes = 4; //we use fixed size of postings (in bytes).
    sets.totalExperimentPostings = globalOpts[gTotalMPostings];
    sets.updatesQuant = 1000*1000;
    sets.percentsUBLeft = 25;

    if(IndexUpdate::HD == disk) {
        //for HD
        sets.tpMembers = {
                21,35,50,69,
                88,109,139,173,
                213,261,323,401,
                489,618,838,1206,
                1776,3208,8213,33396,
                2262672
        };
        //64
        sets.tpQueries = {
                24121,23619,23471,23490,
                23739,23537,23909,23436,
                23638,23529,23558,23565,
                23465,23798,23885,23591,
                23550,23559,23513,23700,
                23826};

        sets.tpUpdates = {
                424141246,422228256,420319852,419876347,
                423493286,419725298,421212817,420285949,
                421057945,420528111,420271342,419493211,
                420077546,419777985,419348858,419560157,
                419308877,419278178,419272221,419296846,
                419312938,
        };
    }
    else {
        sets.tpQueries = {
                11782,11566,11459,11292,
                11286,11110,11094,11304,
                11035,11024,11243,11176,
                11192,11143,11217,11185,
                11003,11021,11299,11217,
                10985,11152,11039,11157,
                11119,10987,11101,11003,
                11288,11245,11276,11299,
                11092,11029,11088,11129,
                11121,11066,11057,11009,
                11202,11239,11134,11326,
                10948	
        };
        sets.tpUpdates = {
                195705322,196349764,195399697,194151434,
                190936261,191261649,190209710,192363674,
                188647608,188619532,188937977,190359610,
                190364053,189135695,189732419,189071817,
                190226539,190044871,190089281,189533200,
                189405216,188879566,188910233,189048263,
                189482970,189194333,189319583,188629060,
                189130332,188608975,188784441,188856017,
                188622468,188843689,188692968,188603141,
                188582983,188682550,188553502,188609506,
                188603367,188585978,188573692,188582030,
                188599756,
        };
        sets.tpMembers = {
                8, 12, 15, 18,
                21,25,29,34,
                37,41,46,51,
                57, 64,71,79,
                87,96,106,116,
                128,141,156,174,
                192,212,231,252,
                288,328,381,444,
                532,637,757,924,
                1213,1704,2583,4273,
                7605,15724,43332,230072,
                57503262	
        };
    }
    return sets;
}


void findOptimal(IndexUpdate::DiskType disk, unsigned queries){
    Settings settings = setup(disk,queries);
    double minTimes =std::numeric_limits<double>::max();
    std::pair<unsigned,unsigned> bestParams;
    for(auto percents : {16,96,32,50} ) { //larger percents ==> larger UB ==> less evictions!
        settings.flags[0] = percents;
        settings.updateBufferPostingsLimit = ((1ull << 31) * percents) / 100;
        settings.cacheSizePostings = (1ull << 31) - settings.updateBufferPostingsLimit;

        for(auto reduceTo : { 90,4, 16, 32,64}) {
            settings.percentsUBLeft = reduceTo;
            settings.flags[1] = reduceTo;
            auto totalTimes = Simulator::simulateOne(SkiBased, settings);
            std::cerr << totalTimes << std::endl;
            if (totalTimes < minTimes) {
                bestParams = std::make_pair(percents, reduceTo);
                minTimes = totalTimes;
            }
        }
    }
    std::cout << bestParams.first << ' ' << bestParams.second << std::endl;
}
