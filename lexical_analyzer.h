#ifndef LEXICAL_ANALYZER_H
#define LEXICAL_ANALYZER_H

#include <bitset>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "token.h"

namespace hcc {

class LexicalAnalyzer {
private:
    enum { kQuote, kCommon, kSpace, kSingle, kNeedless };
    std::vector<int> kind;
    std::map<std::string, bool> keyword_and_symbol;
    std::map<std::string, bool> is_two_character_operator;
    LexicalAnalyzer();
    bool IsTwoCharacterOperator(const std::string&);
    std::vector<Token> Split(std::unique_ptr<std::fstream, FileDeleter>&);
    std::vector<Token> Classify(std::vector<Token>&);

public:
    LexicalAnalyzer(const LexicalAnalyzer&) = delete;
    LexicalAnalyzer& operator=(const LexicalAnalyzer&) = delete;
    ~LexicalAnalyzer();

    static LexicalAnalyzer& Instance(void);
    void Work(std::unique_ptr<std::fstream, FileDeleter>&);
};

} // namespace hcc

#endif