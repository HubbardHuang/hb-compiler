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
    int start_vertex;
    std::unordered_map<int, bool> is_end_vertex;
    std::map<char, int> code_of;
    std::vector<std::vector<Road>> graph;

    int Go(int, char);
    bool AtEnd(int);

public:
    ~DFA();
    DFA() = default;
    DFA(const std::pair<int, const std::vector<int>&>&, const std::vector<XYZ>&,
        const std::map<char, int>&);
    bool Judge(const std::string&);
};

} // namespace hcc

#endif