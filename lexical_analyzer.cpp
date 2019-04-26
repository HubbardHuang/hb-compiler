#include <algorithm>
#include <cctype>
#include <iostream>

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
}

LexicalAnalyzer&
LexicalAnalyzer::Instance(void) {
    static LexicalAnalyzer la;
    return la;
}

std::vector<Token>
LexicalAnalyzer::Split(std::shared_ptr<std::fstream>& source_file) {
    std::vector<Token> result;
    std::string buffer;
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
                    ++column;
                    if (c == '/' && next == '*') {
                        buffer += "/*";
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
                    } else if (c == '/' && next == '/') {
                        buffer += "//";
                        while (source_file->get(next) && next != '\n') {
                            ++column;
                            buffer.push_back(next);
                        }
                        ++row;
                        prev_column = column;
                        column = 0;
                    } else {
                        --column;
                        source_file->seekg(-1, std::fstream::cur);
                        buffer.push_back(c);
                    }
                } else {
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
            } else if (buffer.size() > 2 && buffer[0] == '/' && buffer[1] == '*') {
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

void
LexicalAnalyzer::Work(std::shared_ptr<std::fstream>& source_file) {
    std::vector<Token> result = Split(source_file);
    for (auto t : result) {
        std::cout << t.name << " " << t.length << "\t(" << t.row << ", " << t.column
                  << ")" << std::endl;
    }
}

} // namespace hcc