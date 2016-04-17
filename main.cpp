#include <algorithm>
#include <iostream>
#include <future>

#include "Simulator.h"
#include "Settings.h"

using namespace IndexUpdate;

IndexUpdate::Settings setup(IndexUpdate::DiskType disk = IndexUpdate::HD, unsigned queriesQuant = 64 );

void experiment(IndexUpdate::DiskType disk, unsigned queries);

int main(int argc, char** argv) {
    unsigned queries = (argc>1) ? atoi(argv[1]) : 64;

    experiment(HD, queries);
    std::cout << std::endl;
    experiment(SSD,queries);
    std::cout << std::endl;
    std::cin >> argc;
    return 0;
}

void experiment(IndexUpdate::DiskType disk, unsigned queries){
    Settings settings = setup(disk,queries);
    std::vector<Algorithm> algs {SkiBased, LogMerge}; //{AlwaysMerge, LogMerge};

    typedef std::vector<std::string> ReportT;

    std::vector<std::future<ReportT>> futuresVec;

    for(auto percents : {96,75,50,32,25,16} ) { //larger percents ==> larger UB ==> less evictions!
        settings.updateBufferPostingsLimit = ((1ull << 31) * percents) / 100;
        futuresVec.emplace_back(
                std::async(std::launch::async,Simulator::simulate,
                           std::cref(algs), settings));
    }

    std::vector<std::string> reports;
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
    sets.totalExperimentPostings = 64ull*1000*1000*1000;
    sets.updatesQuant = 1000000;

    //for HD
    sets.tpMembers = {
            21,
            35,
            50,
            69,
            88,
            109,
            139,
            173,
            213,
            261,
            323,
            401,
            489,
            618,
            838,
            1206,
            1776,
            3208,
            8213,
            33396,
            2262672
    };
    /*
    //1024
    sets.tpQueries = {
            384984,
            378992,
            376987,
            377808,
            380313,
            375833,
            379344,
            376866,
            379096,
            377670,
            377535,
            377798,
            376519,
            376351,
            377319,
            377626,
            377226,
            378103,
            378406,
            379228,
            378777
    };
*/

    //64
    sets.tpQueries = {
            24121,
            23619,
            23471,
            23490,
            23739,
            23537,
            23909,
            23436,
            23638,
            23529,
            23558,
            23565,
            23465,
            23798,
            23885,
            23591,
            23550,
            23559,
            23513,
            23700,
            23826	};

    sets.tpUpdates = {
            424141246,
            422228256,
            420319852,
            419876347,
            423493286,
            419725298,
            421212817,
            420285949,
            421057945,
            420528111,
            420271342,
            419493211,
            420077546,
            419777985,
            419348858,
            419560157,
            419308877,
            419278178,
            419272221,
            419296846,
            419312938,
    };

    return sets;
}