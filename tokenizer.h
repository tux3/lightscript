#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <vector>
#include <string>
#include <cstddef>

/// Positive values are reserved for non-token single characters
enum Token : char
{
    tok_eof = -1,
    tok_invalid = -2,

    tok_identifier = -3,

    tok_int_literal = -4,
    tok_float_literal = -5,
    tok_string_literal = -6,

    tok_true = -7,
    tok_false = -8,

    // Type keywords
    tok_int = -10,
    tok_float = -11,
    tok_string = -12,
    tok_bool = -13,
    tok_void = -14,

    tok_extern = -20,
};

struct TokenData
{
    std::string identifier;
    long intValue;
    double floatValue;
};

class Tokenizer
{
public:
    Tokenizer(const std::vector<char>& script);
    ~Tokenizer();

    Token getNextToken();
    Token peekNextToken() const;
    Token getCurToken() const;
    std::string getCurIdentifier() const;
    long getCurIntLiteral() const;
    double getCurFloatLiteral() const;
    size_t getCurLine() const;
    int getCurTokPrecedence() const;

private:
    Token readNextToken(TokenData& data, size_t& pos, size_t& line) const;
    int readNextChar(size_t& pos, size_t& line) const; ///< Returns EOF on error
    int readCurChar(size_t& pos) const; ///< Returns EOF on error

private:
    const std::vector<char>& script;
    size_t curLine, curPos;
    Token curTok;
    TokenData curTokData;
};

#endif // TOKENIZER_H
