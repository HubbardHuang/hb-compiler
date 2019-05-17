#ifndef DFA_H
#define DFA_H

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace hcc {

struct XYZ {
    XYZ(int x, int y, int z);
    int start;
    int end;
    int weight;
};

class DFA {
private:
    using Road = struct Road {
        Road(int n, int w);
        int neighbor;
        int weight;
    };
    int start_vertex_;
    std::unordered_map<int, bool> is_end_vertex_;
    std::map<char, int> code_of_;
    std::vector<std::vector<Road>> graph_;

    int Go(int curr_vertex, char c);
    bool AtEnd(int curr_vertex);

public:
    ~DFA();
    DFA() = default;
    DFA(const std::pair<int, const std::vector<int>&>& start_and_end,
        const std::vector<XYZ>& triad, const std::map<char, int>& bit_of);
    bool Judge(const std::string& unit);
};

} // namespace hcc

#endif