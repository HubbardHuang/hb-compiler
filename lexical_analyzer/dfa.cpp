#include "dfa.h"

namespace hcc {

XYZ::XYZ(int x, int y, int z)
  : start(x)
  , end(y)
  , weight(z) {}

DFA::Road::Road(int n, int w)
  : neighbor(n)
  , weight(w) {}

int
DFA::Go(int curr_vertex, char c) {
    if (graph_[curr_vertex].empty()) {
        return -1;
    }
    for (auto road : graph_[curr_vertex]) {
        if (road.weight & code_of_[c]) {
            return road.neighbor;
        }
    }
    return -1;
}

DFA::~DFA() {}

DFA::DFA(const std::pair<int, const std::vector<int>&>& start_and_end,
         const std::vector<XYZ>& triad, const std::map<char, int>& bit_of) {
    code_of_ = bit_of;

    int vertex_count = 0;
    for (auto t : triad) {
        if (vertex_count < t.start) {
            vertex_count = t.start;
        }
        if (vertex_count < t.end) {
            vertex_count = t.end;
        }
    }
    graph_.resize(vertex_count + 1);
    for (auto t : triad) {
        graph_[t.start].push_back(Road(t.end, t.weight));
    }
    start_vertex_ = start_and_end.first;
    for (auto e : start_and_end.second) {
        is_end_vertex_.insert({ e, true });
    }
}

bool
DFA::AtEnd(int curr_vertex) {
    auto temp = is_end_vertex_.find(curr_vertex);
    if (temp != is_end_vertex_.end()) {
        return true;
    } else {
        return false;
    }
}

bool
DFA::Judge(const std::string& unit) {
    if (unit.empty()) {
        return false;
    }
    int curr_vertex = start_vertex_;
    for (auto ch : unit) {
        int next_vertex;
        if ((next_vertex = Go(curr_vertex, ch)) != -1) {
            curr_vertex = next_vertex;
        } else {
            return false;
        }
    }
    if (AtEnd(curr_vertex)) {
        return true;
    } else {
        return false;
    }
}

} // namespace hcc