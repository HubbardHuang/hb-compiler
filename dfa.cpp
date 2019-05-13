#include "dfa.h"

namespace hcc {

bool
DFA::Judge(const std::string& unit) {
    if (unit.empty()) {
        return false;
    }
    int curr_vertex = start_vertex;
    for (auto ch : unit) {
        if (CanGo(ch)) {

        } else {
            return false;
        }
    }
}

} // namespace hcc