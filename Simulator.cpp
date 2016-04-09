#include "Simulator.h"
#include "Settings.h"

#include <iostream>
#include <chrono>

using namespace IndexUpdate;


class Simulator::SimulatorIMP {
    const Settings settings;

    uint64_t totalSeenPostings;
    uint64_t postingsInUpdateBuffer;

public:
    explicit SimulatorIMP(const Settings& s);
    ~SimulatorIMP() {}
    void execute();

    void init() {}
    bool finished() const;
    bool bufferFull() const;
    void handleQueries();
    void fillUpdateBuffer();
    void evictFromUpdateBuffer();
};

Simulator::Simulator(const Settings& settings) {
    pimpl = new SimulatorIMP(settings);
}

Simulator::~Simulator() {
    delete pimpl;
    pimpl = nullptr;
}

void Simulator::operator()() {
    pimpl->execute();
}

Simulator::SimulatorIMP::SimulatorIMP(const Settings& s) :
        settings(s),
        totalSeenPostings(0),
        postingsInUpdateBuffer(0){
}

void Simulator::SimulatorIMP::execute() {
    auto start = std::chrono::system_clock::now();

    try {
        init();
        while(!finished()) {
            while (!bufferFull()) {
                handleQueries();
                fillUpdateBuffer();
            }
            evictFromUpdateBuffer();
            std::cout << totalSeenPostings << std::endl;
        }
    }
    catch(std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    auto end = std::chrono::system_clock::now();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
    std::cout << "Done ( " << millis << "ms. )\n";
}

void Simulator::SimulatorIMP::handleQueries() {

}

void Simulator::SimulatorIMP::fillUpdateBuffer() {
    postingsInUpdateBuffer += settings.updatesQuant;
}

void Simulator::SimulatorIMP::evictFromUpdateBuffer() {
    totalSeenPostings += postingsInUpdateBuffer;
    postingsInUpdateBuffer = 0;
}

bool Simulator::SimulatorIMP::bufferFull() const {
    return postingsInUpdateBuffer >= settings.updateBufferPostingsLimit;
}

bool Simulator::SimulatorIMP::finished() const {
    return totalSeenPostings >= settings.totalExperimentPostings;
}

