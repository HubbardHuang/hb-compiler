#ifndef TOKEN_H
#define TOKEN_H

#include <string>

namespace hcc {

enum class Morpheme {
    // Keyword about type
    kConst,
    kStatic,
    kLong,
    kInt,
    kChar,
    kShort,
    kFloat,
    kDouble,
    kSigned,
    kUnsigned,
    kBool,
    kVoid,
    kString,
    // Keyword about control structure
    kFor,
    kWhile,
    kIf,
    kElse,
    kDo,
    kCase,
    kSwitch,
    kContinue,
    kBreak,
    // name of variable and function
    kId,
    // Operator
    kPlusSign,
    kMinusSign,
    kMultiplicationSign,
    kDivisionSign,
    kBracketLeft,
    kBracketRight,
    // other
    kParenthesesLeft,
    kParenthesesRihgt,
    kBraceLeft,
    kBraceRight,
};

struct Token {
    Token(size_t r, size_t c, size_t l, const std::string& n)
      : row(r)
      , column(c)
      , length(l)
      , name(n) {}
    size_t row;
    size_t column;
    size_t length;
    std::string name;
};

}; // namespace hcc

#endif