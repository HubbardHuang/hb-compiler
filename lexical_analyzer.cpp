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
  : kind_(129, kNeedless)
  , unit_type_({ "comment", "number", "id", "character", "string" })
  , number_pattern_("\\d+(.(\\d+))?(E[+-]?(\\d+))?")
  , id_pattern_("[a-zA-Z_][a-zA-Z_0-9]*") {

    // Classify text characters with array [kind_]
    std::string space = " \t\r\n\v\f";
    std::string single = "!%&()*+,-/:;<=>?[]^{}|~";
    std::string common = "#.\\_0123456789";
    for (char c = 'a'; c <= 'z'; c++) {
        common.push_back(c);
        common.push_back(c ^ (1 << 5));
    }
    std::string quote = "\"\'";

    for (auto ch : space) {
        kind_[ch] = kSpace;
    }
    for (auto ch : single) {
        kind_[ch] = kSingle;
    }
    for (auto ch : common) {
        kind_[ch] = kCommon;
    }
    for (auto ch : quote) {
        kind_[ch] = kQuote;
    }

    // Set status number for keyword and symbol
    std::unique_ptr<std::ifstream, FileDeleter> file_of_keyword_and_symbol(
      new std::ifstream("keyword_and_symbol.txt"), FileDeleter());
    std::string buffer;
    while (*file_of_keyword_and_symbol >> buffer) {
        keyword_and_symbol_.insert({ buffer, true });
    }
    for (auto i : keyword_and_symbol_) {
        if (isalpha(i.first.front())) {
            continue;
        }
        if (i.first.size() == 2) {
            is_two_character_operator_.insert({ i.first, true });
        }
    }

    // Initialize id DFA
    std::map<char, int> code_of_id_DFA;
    code_of_id_DFA.insert({ '_', 1 << 0 });
    for (char ch = 'a'; ch <= 'z'; ch++) {
        code_of_id_DFA.insert({ ch, 1 << 0 });
        code_of_id_DFA.insert({ toupper(ch), 1 << 0 });
    }
    for (char ch = '0'; ch <= '9'; ch++) {
        code_of_id_DFA.insert({ ch, 1 << 1 });
    }
    id_machine_ =
      DFA({ 0, { 1 } }, { XYZ(0, 1, 1 << 0), XYZ(1, 1, (1 << 0) | (1 << 1)) },
          code_of_id_DFA);

    // Initialize number DFA
    std::map<char, int> code_of_number_DFA;
    code_of_number_DFA.insert({ '.', 1 << 0 });
    code_of_number_DFA.insert({ 'E', 1 << 1 });
    code_of_number_DFA.insert({ 'e', 1 << 1 });
    code_of_number_DFA.insert({ '+', 1 << 2 });
    code_of_number_DFA.insert({ '-', 1 << 3 });
    for (char ch = '0'; ch <= '9'; ch++) {
        code_of_number_DFA.insert({ ch, 1 << 4 });
    }
    std::vector<XYZ> xyz_buffer;
    xyz_buffer.push_back(XYZ(0, 1, 1 << 4));
    xyz_buffer.push_back(XYZ(1, 1, 1 << 4));
    xyz_buffer.push_back(XYZ(1, 2, 1 << 0));
    xyz_buffer.push_back(XYZ(2, 4, 1 << 4));
    xyz_buffer.push_back(XYZ(4, 4, 1 << 4));
    xyz_buffer.push_back(XYZ(4, 3, 1 << 1));
    xyz_buffer.push_back(XYZ(1, 3, 1 << 1));
    xyz_buffer.push_back(XYZ(3, 6, 1 << 2));
    xyz_buffer.push_back(XYZ(3, 6, 1 << 3));
    xyz_buffer.push_back(XYZ(3, 5, 1 << 4));
    xyz_buffer.push_back(XYZ(6, 5, 1 << 4));
    xyz_buffer.push_back(XYZ(5, 5, 1 << 4));
    number_machine_ = DFA({ 0, { 1, 4, 5 } }, xyz_buffer, code_of_number_DFA);
}

LexicalAnalyzer&
LexicalAnalyzer::Instance(void) {
    static LexicalAnalyzer la;
    return la;
}

bool
LexicalAnalyzer::IsTwoCharacterOperator(const std::string& s) {
    return is_two_character_operator_.find(s) !=
           is_two_character_operator_.end();
}

std::list<Token>
LexicalAnalyzer::Split(
  std::unique_ptr<std::fstream, FileDeleter>& source_file) {
    std::list<Token> result;
    std::string buffer;
    std::string combination;
    char c;
    char begin;
    char next;
    int i;
    int point_count;
    bool go_on_reading = true;
    size_t row = 1;
    size_t column = 0;
    size_t prev_column;
    size_t special_row, special_column; // Used for "/*...*/"
    while (source_file->get(c)) {
        // Find a meaningful morpheme
        switch (kind_[c]) {
        case kCommon:
            do {
                if (kind_[c] != kCommon) {
                    if (c == '+' ||
                        c == '-') { // '-' can be in either "1E-3" or "p->age"
                        point_count = 0;
                        go_on_reading = true;
                        for (i = 0; i < buffer.size(); i++) {
                            if (buffer[i] == '.' && point_count == 0) {
                                ++point_count;
                                continue;
                            } else if (isdigit(buffer[i])) {
                                continue;
                            } else {
                                // --column;
                                // source_file->seekg(-1, std::fstream::cur);
                                // source_file->get(c);
                                go_on_reading = false;
                                break;
                            }
                        }
                        if (!go_on_reading) {
                            break;
                        }
                    } else {
                        break;
                    }
                }
                ++column;
                buffer.push_back(c);
            } while (source_file->get(c));
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
            while (source_file->get(c) && kind_[c] == kSpace) {
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
LexicalAnalyzer::MatchId(const std::string& unit) {
    return id_machine_.Judge(unit);
}

bool
LexicalAnalyzer::MatchNumber(const std::string& unit) {
    return number_machine_.Judge(unit);
}

bool
LexicalAnalyzer::MatchComment(const std::string& unit) {
    auto length = unit.size();
    if (length >= 2 && unit[0] == '/' && unit[1] == '/') {
        return true;
    } else if (length >= 4 && unit[0] == '/' && unit[1] == '*' &&
               unit[length - 1] == '/' && unit[length - 2] == '*') {
        return true;
    }
    return false;
}

bool
LexicalAnalyzer::MatchCharacter(const std::string& unit) {
    auto length = unit.size();
    if (length >= 3 && unit.front() == '\'' && unit.back() == '\'') {
        return true;
    } else {
        return false;
    }
}

bool
LexicalAnalyzer::MatchString(const std::string& unit) {
    auto length = unit.size();
    if (length >= 2 && unit.front() == '\"' && unit.back() == '\"') {
        return true;
    } else {
        return false;
    }
}

bool
LexicalAnalyzer::Match(const std::string& type, const std::string& unit) {
    if (unit.empty()) {
        return false;
    }
    bool match_or_not;
    if (type == "comment") {
        match_or_not = MatchComment(unit);
    } else if (type == "id") {
        match_or_not = id_machine_.Judge(unit);
    } else if (type == "number") {
        match_or_not = MatchNumber(unit);
    } else if (type == "character") {
        match_or_not = MatchCharacter(unit);
    } else if (type == "string") {
        match_or_not = MatchString(unit);
    }
    return match_or_not;
}

void
LexicalAnalyzer::HandleMemberAccessing(std::list<Token>& source) {
    for (auto t = source.begin(); t != source.end(); t++) {
        if (t->type != "OTHER") {
            continue;
        }
        auto found_pos = t->name.find('.');
        if (found_pos != std::string::npos) {
            std::string left = t->name.substr(0, found_pos);
            std::string right = t->name.substr(found_pos + 1);
            // if (isId(left)) {

            //     source.insert(t, Token())
            // }
        }
    }
}

std::list<Token>
LexicalAnalyzer::Classify(std::list<Token>& source) {
    for (auto& i : source) {
        if (keyword_and_symbol_.find(i.name) != keyword_and_symbol_.end()) {
            i.type = i.name;
        } else {
            for (auto type : unit_type_) {
                if (Match(type, i.name)) {
                    i.type = type;
                    break;
                } else {
                    i.type = "OTHER";
                }
            }
        }
    }
    // HandleMemberAccessing(source);

    return source;
}

void
LexicalAnalyzer::Work(std::unique_ptr<std::fstream, FileDeleter>& source_file) {
    tokens_ = Split(source_file);
    // for (auto t : tokens_) {
    //     std::cout << t.name << " " << t.length << "\t(" << t.row << ", "
    //               << t.column << ")" << std::endl;
    // }
    Classify(tokens_);
    for (auto t : tokens_) {
        std::cout << "<" << t.name << ", " << t.type << ">" << std::endl;
    }
}

} // namespace hcc