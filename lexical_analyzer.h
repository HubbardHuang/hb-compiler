#ifndef LEXICAL_ANALYZER_H
#define LEXICAL_ANALYZER_H

#include <bitset>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "token.h"

namespace hcc {

class LexicalAnalyzer {
private:
    enum { kQuote, kCommon, kSpace, kSingle, kNeedless };
    std::vector<int> kind;
    LexicalAnalyzer();
    std::vector<Token> Split(std::shared_ptr<std::fstream>&);

public:
    LexicalAnalyzer(const LexicalAnalyzer&) = delete;
    LexicalAnalyzer& operator=(const LexicalAnalyzer&) = delete;
    ~LexicalAnalyzer();

    static LexicalAnalyzer& Instance(void);
    void Work(std::shared_ptr<std::fstream>&);
};

} // namespace hcc

#endif