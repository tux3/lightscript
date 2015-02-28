#include "tokenizer.h"
#include <map>

using namespace std;

Tokenizer::Tokenizer(const std::vector<char>& Script)
    : script{Script}, curLine{0}, curPos{0}, curTok{tok_eof}
{

}

Tokenizer::~Tokenizer()
{

}

int Tokenizer::readNextChar(size_t& pos, size_t &line) const
{
    if (++pos >= script.size())
        return EOF;
    else
    {
        if (script[pos] == '\n')
            line++;
        return script[pos];
    }
}

int Tokenizer::readCurChar(size_t& pos) const
{
    if (pos >= script.size())
        return EOF;
    else
        return script[pos];
}

Token Tokenizer::readNextToken(TokenData &data, size_t& pos, size_t &line) const
{
    int curChar = readCurChar(pos);
    while (isspace(curChar))
        curChar = readNextChar(pos, line);
    if (curChar == EOF)
        return tok_eof;

    if (curChar == '#')
    {
        // Comment until end of line.
        do {
            curChar = readNextChar(pos, line);
        } while (curChar != EOF && curChar != '\n' && curChar != '\r');

        if (curChar != EOF)
            return readNextToken(data, pos, line);
    }

    if (curChar == '"')
    {
        data.identifier.clear();
        while (1)
        {
            curChar = readNextChar(pos, line);
            if (curChar == '\n' || curChar == '\r')
                return tok_invalid;
            else if (curChar == '"')
            {
                readNextChar(pos, line);
                return tok_string_literal;
            }
            else
                data.identifier += curChar;
        }
    }

    // identifier: [a-zA-Z][a-zA-Z0-9]*
    if (isalpha(curChar))
    {
      data.identifier = curChar;
      while (isalnum((curChar = readNextChar(pos, line))))
        data.identifier += curChar;

      if (data.identifier == "int") return tok_int;
      else if (data.identifier == "float") return tok_float;
      else if (data.identifier == "string") return tok_string;
      else if (data.identifier == "bool") return tok_bool;
      else if (data.identifier == "true") return tok_true;
      else if (data.identifier == "false") return tok_false;
      else if (data.identifier == "void") return tok_void;
      else if (data.identifier == "extern") return tok_extern;
      else return tok_identifier;
    }

    // Number literal: [0-9.]+
    else if (isdigit(curChar) || curChar == '.')
    {
        data.identifier.clear();
        do {
            data.identifier += curChar;
            curChar = readNextChar(pos, line);
        } while (isdigit(curChar) || curChar == '.');

        string::size_type firstDotPos = data.identifier.find('.');
        if (firstDotPos == string::npos)
        {
            data.intValue = strtol(data.identifier.c_str(), 0, 10);
            return tok_int_literal;
        }
        else
        {
            if (data.identifier.find_last_of('.') != firstDotPos)
                return tok_invalid;

            data.floatValue = strtod(data.identifier.c_str(), 0);
            return tok_float_literal;
        }
    }
    else
    {
        readNextChar(pos, line);
        return (Token)curChar;
    }
}

Token Tokenizer::getNextToken()
{
    curTok = readNextToken(curTokData, curPos, curLine);
    return curTok;
}

Token Tokenizer::peekNextToken() const
{
    size_t tmpPos = curPos, tmpLine = curLine;
    TokenData tmpData;
    return readNextToken(tmpData, tmpPos, tmpLine);
}

size_t Tokenizer::getCurLine() const
{
    return curLine;
}

Token Tokenizer::getCurToken() const
{
    return curTok;
}

string Tokenizer::getCurIdentifier() const
{
    return curTokData.identifier;
}

long Tokenizer::getCurIntLiteral() const
{
    return curTokData.intValue;
}

double Tokenizer::getCurFloatLiteral() const
{
    return curTokData.floatValue;
}

/// getTokPrecedence - Get the precedence of the pending binary operator token.
int Tokenizer::getCurTokPrecedence() const
{
  if (!isascii(curTok))
    return -1;

  static std::map<char, int> opPrecedence = {{';', 2}, {'<',10}, {'+',20}, {'-',20}, {'*',40}};

  // Make sure it's a declared binop.
  int tokPrec = opPrecedence[curTok];
  if (tokPrec <= 0) return -1;
  return tokPrec;
}
