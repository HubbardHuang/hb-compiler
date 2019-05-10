#include <algorithm>
#include <cctype>
#include <functional>
#include <iostream>
#include <list>
#include <regex>

#include "file_deleter.h"
#include "lexical_analyzer.h"

namespace hcc {

LexicalAnalyzer::~LexicalAnalyzer() {}

LexicalAnalyzer::LexicalAnalyzer() {
    kind.resize(129);
    std::fill(kind.begin(), kind.end(), kNeedless);

    // Classify text characters with array "kind"
    std::string space = " \t\r\n\v\f";
    std::string single = "!%&()*+,-/:;<=>?[]^{}|~";
    std::string common = "#.\\_0123456789";
    for (char c = 'a'; c <= 'z'; c++) {
        common.push_back(c);
        common.push_back(c ^ (1 << 5));
    }
    std::string quote = "\"\'";

    for (auto ch : space) {
        kind[ch] = kSpace;
    }
    for (auto ch : single) {
        kind[ch] = kSingle;
    }
    for (auto ch : common) {
        kind[ch] = kCommon;
    }
    for (auto ch : quote) {
        kind[ch] = kQuote;
    }

    // Set status number for keyword and symbol
    std::unique_ptr<std::ifstream, FileDeleter> file_of_keyword_and_symbol(
      new std::ifstream("keyword_and_symbol.txt"), FileDeleter());
    std::string buffer;
    while (*file_of_keyword_and_symbol >> buffer) {
        keyword_and_symbol.insert({ buffer, true });
    }
    for (auto i : keyword_and_symbol) {
        if (isalpha(i.first.front())) {
            continue;
        }
        // switch (i.first.size()) {
        // case 2:
        //     is_two_character_operator.insert({ i.first, true });
        //     break;
        // default:
        //     break;
        // }
        if (i.first.size() == 2) {
            is_two_character_operator.insert({ i.first, true });
        }
    }
}

LexicalAnalyzer&
LexicalAnalyzer::Instance(void) {
    static LexicalAnalyzer la;
    return la;
}

bool
LexicalAnalyzer::IsTwoCharacterOperator(const std::string& s) {
    return is_two_character_operator.find(s) != is_two_character_operator.end();
}

std::vector<Token>
LexicalAnalyzer::Split(
  std::unique_ptr<std::fstream, FileDeleter>& source_file) {
    std::vector<Token> result;
    std::string buffer;
    std::string combination;
    char c;
    char begin;
    char next;
    size_t row = 1;
    size_t column = 0;
    size_t prev_column;
    size_t special_row, special_column; // Used for "/*...*/"
    while (source_file->get(c)) {
        // Find a meaningful morpheme
        switch (kind[c]) {
        case kCommon:
            do {
                ++column;
                buffer.push_back(c);
            } while (source_file->get(c) && kind[c] == kCommon);
            source_file->seekg(-1, std::fstream::cur);
            break;
        case kQuote:
            ++column;
            buffer.push_back(c);
            begin = c;
            while (source_file->get(c)) {
                ++column;
                buffer.push_back(c);
                if (c == '\\') {
                    if (source_file->get(c)) {
                        ++column;
                        buffer.push_back(c);
                    }
                } else if (c == begin) {
                    break;
                }
            }
            break;
        case kSingle:
            ++column;
            if (source_file->get(next)) {
                combination.push_back(c);
                combination.push_back(next);
                ++column;
                if (combination == "/*") { // Comment, "/* .. */"
                    buffer += combination;
                    special_column = column - 1;
                    special_row = row;
                    while (source_file->get(next)) {
                        if (next == '\n') {
                            ++row;
                            prev_column = column;
                            column = 0;
                        }
                        buffer.push_back(next);
                        if (c == '*' && next == '/') {
                            break;
                        }
                        c = next;
                    }
                } else if (combination == "//") { // Comment, "// ..."
                    buffer += combination;
                    while (source_file->get(next) && next != '\n') {
                        ++column;
                        buffer.push_back(next);
                    }
                    ++row;
                    prev_column = column;
                    column = 0;
                } // Operator consisting of 3 characters, only "<<=" and ">>="
                else if (combination == "<<" || combination == ">>") {
                    if (source_file->get(next)) {
                        if (next == '=') {
                            ++column;
                            combination += next;
                        } else {
                            --column;
                            source_file->seekg(-1, std::fstream::cur);
                        }
                    }
                    buffer += combination;
                } // Operator consisting of 2 characters
                else if (IsTwoCharacterOperator(combination)) {
                    buffer += combination;
                } else { // Operator consisting of 1 characters
                    --column;
                    source_file->seekg(-1, std::fstream::cur);
                    buffer.push_back(c);
                }
                combination.clear();
            } else { // Fail to read next character from the source file
                --column;
                source_file->seekg(-1, std::fstream::cur);
                buffer.push_back(c);
            }
            break;
        case kSpace:
            ++column;
            if (c == '\n') {
                prev_column = column;
                column = 0;
                ++row;
            }
            while (source_file->get(c) && kind[c] == kSpace) {
                ++column;
                if (c == '\n') {
                    prev_column = column;
                    column = 0;
                    ++row;
                }
            }
            source_file->seekg(-1, std::fstream::cur);
            break;
        default:
            break;
        }

        // Write it into the buffer
        if (!buffer.empty()) {
            if (buffer.size() > 2 && buffer[0] == '/' && buffer[1] == '/') {
                int r = row - 1;
                int c = (column ? (column - buffer.size() + 1)
                                : (prev_column - buffer.size() + 1));
                result.push_back(Token(r, c, buffer.size(), buffer));
            } else if (buffer.size() > 2 && buffer[0] == '/' &&
                       buffer[1] == '*') {
                int r = special_row;
                int c = special_column;
                result.push_back(Token(r, c, buffer.size(), buffer));
            } else {
                int r = row;
                int c = (column ? (column - buffer.size() + 1)
                                : (prev_column - buffer.size() + 1));
                result.push_back(Token(r, c, buffer.size(), buffer));
            }
            buffer.clear();
        }
    }
    return result;
} // namespace hcc

static bool
MatchHeader(const std::string& s) {
    if (s.empty()) {
        return false;
    }
    return true;
}

static bool
MatchId(const std::string& s) {
    if (s.empty()) {
        return false;
    }
    if (s.front() != '_' && !isalpha(s.front())) {
        return false;
    }
    for (auto ch : s) {
        if (isalnum(ch) || ch == '_') {
            continue;
        } else {
            return false;
        }
    }
    return true;
}

static bool
MatchNumber(const std::string& s) {
    return std::regex_match(s, std::regex("\\d+(.(\\d+))?(E[+-]?(\\d+))?"));
}

std::vector<Token>
LexicalAnalyzer::Classify(std::vector<Token>& source) {
    for (auto& i : source) {
        if (keyword_and_symbol.find(i.name) != keyword_and_symbol.end()) {
            i.type = i.name;

        }
        // function name, variable name, string, character
        else if (i.name.front() == '\"' && i.name.back() == '\"') {
            i.type = "string";
        } else if (i.name.size() >= 2 &&
                   (i.name[0] == '/' && i.name[1] == '/' ||
                    i.name[0] == '/' && i.name[1] == '*')) {
            i.type = "comment";
        } else if (i.name.front() == '\'' && i.name.back() == '\'') {
            i.type = "character";
        } else if (MatchId(i.name)) {
            i.type = "id";
        } else if (MatchNumber(i.name)) {
            i.type = "num";
        } else {
            i.type = "ERROR";
        }
    }
    for (int i = 0; i < source.size(); i++) {
        if (source[i].name == "#include") {
            for (int left = i, right = i + 1; right < source.size();
                 left++, right++, i = right) {
                if (source[left].name == "<" || source[left].name == "\"") {
                    source[right].type = "header";
                }
            }
        }
    }
    return source;
}

void
LexicalAnalyzer::Work(std::unique_ptr<std::fstream, FileDeleter>& source_file) {
    std::vector<Token> first_step_result = Split(source_file);
    for (auto t : first_step_result) {
        std::cout << t.name << " " << t.length << "\t(" << t.row << ", "
                  << t.column << ")" << std::endl;
    }
    // std::vector<Token> second_step_result = Classify(first_step_result);
    // for (auto t : second_step_result) {
    //     std::cout << "<" << t.name << ", " << t.type << ">" << std::endl;
    // }
} // namespace hcc

} // namespace hcc