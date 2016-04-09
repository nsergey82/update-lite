#ifndef UPDATE_LITE_SIMULATOR_H
#define UPDATE_LITE_SIMULATOR_H

namespace IndexUpdate {
    class Settings;

    class Simulator {
        class SimulatorIMP;
    public:
        explicit Simulator(const Settings&);
        ~Simulator();
        void operator()();
    private:
        SimulatorIMP *pimpl;
    };
}

#endif //UPDATE_LITE_SIMULATOR_H
