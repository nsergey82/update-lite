#include "Simulator.h"
#include "Settings.h"

int main() {
    using namespace IndexUpdate;

    Settings sets;
    sets.updateBufferPostingsLimit = (256*1000*1000);
    sets.totalExperimentPostings = 64ull*1000*1000*1000;
    sets.quieriesQuant = 2;
    sets.updatesQuant = 1000000;

    sets.tpMembers = { 10000,10000};
    sets.tpQueries = { 20123,10000};
    sets.tpUpdates = { 100000,100000};
    Simulator::simulate(sets);

    return 0;
}