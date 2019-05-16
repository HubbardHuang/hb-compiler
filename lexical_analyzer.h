#ifndef LEXICAL_ANALYZER_H
#define LEXICAL_ANALYZER_H

#include <bitset>
#include <fstream>
#include <map>
#include <memory>
#include <regex>
#include <string>
#include <vector>

#include "dfa.h"
#include "token.h"

namespace hcc {

class LexicalAnalyzer {
private:
    enum { kQuote, kCommon, kSpace, kSingle, kNeedless };
    std::vector<int> kind;
    std::map<std::string, bool> keyword_and_symbol;
    std::map<std::string, bool> is_two_character_operator;

    std::vector<std::string> morpheme_type;
    std::string number_pattern;
    std::string id_pattern;
    DFA id_machine;
    DFA number_machine;

    LexicalAnalyzer();

    bool IsTwoCharacterOperator(const std::string&);
    bool MatchId(const std::string& unit);
    bool Match(const std::string& type, const std::string& unit);

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