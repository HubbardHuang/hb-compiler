#include <algorithm>
#include <cctype>
#include <functional>
#include <iostream>
#include <list>

#include "file_deleter.h"
#include "lexical_analyzer.h"

namespace hcc {

LexicalAnalyzer::~LexicalAnalyzer() {}

LexicalAnalyzer::LexicalAnalyzer()
  : kind(129, kNeedless)
  , morpheme_type({ "comment", "number", "id", "character", "string" })
  , number_pattern("\\d+(.(\\d+))?(E[+-]?(\\d+))?")
  , id_pattern("[a-zA-Z_][a-zA-Z_0-9]*") {
    // kind.resize(129);
    // std::fill(kind.begin(), kind.end(), kNeedless);

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
        if (i.first.size() == 2) {
            is_two_character_operator.insert({ i.first, true });
        }
    }

    // Initialize DFA
    std::map<char, int> code_of_number_DFA;
    code_of_number_DFA.insert({ '_', 1 << 0 });
    for (char ch = 'a'; ch <= 'z'; ch++) {
        code_of_number_DFA.insert({ ch, 1 << 0 });
        code_of_number_DFA.insert({ toupper(ch), 1 << 0 });
    }
    for (char ch = '0'; ch <= '9'; ch++) {
        code_of_number_DFA.insert({ ch, 1 << 1 });
    }
    id_machine =
      DFA({ 0, 1 }, { XYZ(0, 1, 1 << 0), XYZ(1, 1, (1 << 0) | (1 << 1)) },
          code_of_number_DFA);
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
}

bool
LexicalAnalyzer::Match(const std::string& type, const std::string& unit) {
    if (unit.empty()) {
        return false;
    }
    auto length = unit.size();
    if (type == "comment") {
        if (length >= 2 && unit[0] == '/' && unit[1] == '/') {
            return true;
        } else if (length >= 4 && unit[0] == '/' && unit[1] == '*' &&
                   unit[length - 1] == '/' && unit[length - 2] == '*') {
            return true;
        }
    } else if (type == "id") {
        // bool match = std::regex_match(unit, std::regex(id_pattern));
        bool match = id_machine.Judge(unit);
        if (match) {
            return true;
        }
    } else if (type == "number") {
        bool match = std::regex_match(unit, std::regex(number_pattern));
        if (match) {
            return true;
        }
    } else if (type == "character") {
        if (length >= 3 && unit.front() == '\'' && unit.back() == '\'') {
            return true;
        }
    } else if (type == "string") {
        if (length >= 2 && unit.front() == '\"' && unit.back() == '\"') {
            return true;
        }
    }
    return false;
}

std::vector<Token>
LexicalAnalyzer::Classify(std::vector<Token>& source) {
    for (auto& i : source) {
        if (keyword_and_symbol.find(i.name) != keyword_and_symbol.end()) {
            i.type = i.name;
        } else {
            for (auto type : morpheme_type) {
                if (Match(type, i.name)) {
                    i.type = type;
                    break;
                } else {
                    i.type = "OTHER";
                }
            }
        }
    }

    return source;
}

void
LexicalAnalyzer::Work(std::unique_ptr<std::fstream, FileDeleter>& source_file) {
    std::vector<Token> first_step_result = Split(source_file);
    // for (auto t : first_step_result) {
    //     std::cout << t.name << " " << t.length << "\t(" << t.row << ", "
    //               << t.column << ")" << std::endl;
    // }
    std::vector<Token> second_step_result = Classify(first_step_result);
    for (auto t : second_step_result) {
        std::cout << "<" << t.name << ", " << t.type << ">" << std::endl;
    }
}

} // namespace hcc