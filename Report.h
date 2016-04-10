#ifndef UPDATE_LITE_REPORT_H
#define UPDATE_LITE_REPORT_H

#include "Settings.h"
#include <string>
#include <unordered_map>

namespace IndexUpdate {
    class Experiment;
    class Report {
    public:
        Report(){}
        //manipulators: fix the settings and alg
        Report& operator<<(const Settings& settings);
        Report& operator<<(Algorithm alg);

        //expected to come in pairs: key, value
        Report& operator<<(const std::string& key);
        Report& operator<<(double value);
        Report& operator<<(uint64_t value);

        void printTo(std::ostream& out) const;

    private:
       std::unordered_map<Settings, Experiment*> experiments;
    };
}


#endif //UPDATE_LITE_REPORT_H
