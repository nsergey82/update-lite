#include "Simulator.h"
#include "Settings.h"

int main() {
    using namespace IndexUpdate;

    Settings sets;

    sets.ioMBS = 150; //how many MB per second we can read/write
    sets.ioSeek = 7; //the time to make an average seek (random access latency)
    sets.szOfPostingBytes = 4; //we use fixed size of postings (in bytes).

    sets.totalExperimentPostings = 64ull*1000*1000*1000;
    sets.quieriesQuant = 64;
    sets.updatesQuant = 1000000;

    //for HD
    sets.tpMembers = { 10000,10000};
    sets.tpQueries = { 20123,10000};
    sets.tpUpdates = { 100000,100000};

    for(auto percents : {16,25,32,50,75,96} ) {
        sets.updateBufferPostingsLimit = ((1ull << 31) * percents) / 100;
        Simulator::simulate(sets);
    }

    return 0;
}