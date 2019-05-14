#include "dfa.h"

namespace hcc {

int
DFA::Go(int curr_vertex, char c) {
    if (graph[curr_vertex].empty()) {
        return -1;
    }
    for (auto road : graph[curr_vertex]) {
        if (road.weight & code_of[c]) {
            return road.neighbor;
        }
    }
    return -1;
}

DFA::~DFA() {}

DFA::DFA(const std::vector<XYZ>& triad, const std::map<char, int>& bit_of) {
    code_of = bit_of;

    int vertex_count = 0;
    for (auto t : triad) {
        if (vertex_count < t.start) {
            vertex_count = t.start;
        }
        if (vertex_count < t.end) {
            vertex_count = t.end;
        }
    }
    graph.resize(vertex_count + 1);
    for (auto t : triad) {
        graph[t.start].push_back(Road(t.end, t.weight));
    }

    for (int i = 0; i < graph.size(); i++) {
        if (graph[i].empty()) {
            end_vertex = i;
            break;
        }
    }

    const int kTrue = 1;
    const int kFalse = 0;
    vector<int> be_neighbor(vertex_count, kFalse);
    for (auto g : graph) {
        for (auto v : g) {
            be_neighbor[v.neighbor] = kTrue;
        }
    }
    for (int i = 0; i < be_neighbor.size(); i++) {
        if (!be_neighbor[i]) {
            start_vertex = i;
        }
    }
}

bool
DFA::Judge(const std::string& unit) {
    if (unit.empty()) {
        return false;
    }
    int curr_vertex = start_vertex;
    for (auto ch : unit) {
        int next_vertex;
        if ((next_vertex = Go(curr_vertex, ch)) != -1) {
            curr_vertex = next_vertex;
        } else if (curr_vertex == end_vertex) {
            break;
        } else {
            return false;
        }
    }
    return true;
}

} // namespace hcc