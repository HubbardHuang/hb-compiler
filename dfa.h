#ifndef DFA_H
#define DFA_H

#include <map>
#include <string>
#include <vector>

namespace hcc {

class DFA {
private:
    int start_vertex;
    int end_vertex;
    std::vector<std::vector<int>> graph;
    int DecodeWeight(int);
    bool CanGo(char ch);

public:
    bool Judge(const std::string&);
};

} // namespace hcc

#endif