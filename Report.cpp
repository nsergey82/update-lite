#include "Report.h"

namespace IndexUpdate {
    //manipulators: fix the settings and alg
    Report& Report::operator<<(const Settings& settings) {
        return *this;
    }

    Report& Report::operator<<(Algorithm alg) {
        return *this;
    }

    //expected to come in pairs: key, value
    Report& Report::operator<<(const std::string& key) {
        return *this;
    }

    Report& Report::operator<<(double value) {
        return *this;
    }

    Report& Report::operator<<(uint64_t value) {
        return *this;
    }

    void Report::printTo(std::ostream& out) const {

    }

}
