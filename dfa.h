#ifndef DFA_H
#define DFA_H

#include <map>
#include <string>
#include <vector>

namespace hcc {

struct XYZ {
    int start;
    int end;
    int weight;
};

class DFA {
private:
    using Road = struct Road {
        int neighbor;
        int weight;
    };
    int start_vertex;
    int end_vertex;
    std::map<char, int> code_of;
    std::vector<std::vector<Road>> graph;

    // int DecodeWeight(int);
    int Go(int, char);

public:
    ~DFA();
    DFA(const std::vector<XYZ>&, const std::map<char, int>&);
    bool Judge(const std::string&);
};

} // namespace hcc

#endif