#ifndef LEXICAL_ANALYZER_H
#define LEXICAL_ANALYZER_H

#include <bitset>
#include <fstream>
#include <functional>
#include <list>
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
    std::vector<int> kind_;
    std::map<std::string, bool> keyword_and_symbol_;
    std::map<std::string, bool> is_two_character_operator_;

    std::vector<std::string> unit_type_;
    std::string number_pattern_;
    std::string id_pattern_;
    DFA id_machine_;
    DFA number_machine_;
    std::map<std::string, std::function<bool(const std::string&)>>
      match_function_of;

    std::list<Token> tokens_;

    LexicalAnalyzer();

    bool IsTwoCharacterOperator(const std::string& s);
    bool MatchId(const std::string& unit);
    bool MatchNumber(const std::string& unit);
    bool MatchComment(const std::string& unit);
    bool MatchCharacter(const std::string& unit);
    bool MatchString(const std::string& unit);
    bool Match(const std::string& type, const std::string& unit);
    void HandleMemberAccessing(std::list<Token>& source);

    std::list<Token> Split(
      std::unique_ptr<std::fstream, FileDeleter>& source_file);
    std::list<Token> Classify(std::list<Token>& source);

public:
    LexicalAnalyzer(const LexicalAnalyzer&) = delete;
    LexicalAnalyzer& operator=(const LexicalAnalyzer&) = delete;
    ~LexicalAnalyzer();

    static LexicalAnalyzer& Instance(void);
    void Work(std::unique_ptr<std::fstream, FileDeleter>& source_file);
};

} // namespace hcc

#endif