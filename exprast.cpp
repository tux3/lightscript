#include "exprast.h"
#include "codegen.h"
#include "tokenizer.h"
#include <cstdlib>

using namespace llvm;

/// Error* - These are little helper functions for error handling.
ExprAST *ASTParser::error(const char *str) { fprintf(stderr, "Error on line %li: %s\n", tokenizer.getCurLine(), str);return 0;}
PrototypeAST *ASTParser::errorP(const char *str) { error(str); return 0; }
FunctionAST *ASTParser::errorF(const char *str) { error(str); return 0; }

ExprAST::ExprAST()
{
}

ExprAST::~ExprAST()
{
}

ASTParser::ASTParser(Tokenizer& Tokenizer)
    : tokenizer{Tokenizer}
{
}

Value* IntLitExprAST::codegen(CodeGen &gen)
{
    return gen.codegen(this);
}

Value* FloatLitExprAST::codegen(CodeGen &gen)
{
    return gen.codegen(this);
}

Value* StringLitExprAST::codegen(CodeGen &gen)
{
    return gen.codegen(this);
}

Value* BoolLitExprAST::codegen(CodeGen &gen)
{
    return gen.codegen(this);
}

Value* VariableExprAST::codegen(CodeGen &gen)
{
    return gen.codegen(this);
}

Value* BinaryExprAST::codegen(CodeGen &gen)
{
    return gen.codegen(this);
}

Value* CallExprAST::codegen(CodeGen &gen)
{
    return gen.codegen(this);
}

Value* VoidExprAST::codegen(CodeGen &gen)
{
    return gen.codegen(this);
}

Value* UnaryExprAST::codegen(CodeGen &gen)
{
    return gen.codegen(this);
}

Value* SequenceExprAST::codegen(CodeGen &gen)
{
    return gen.codegen(this);
}

Value* IfExprAST::codegen(CodeGen &gen)
{
    return gen.codegen(this);
}

/// numberexpr ::= number
ExprAST* ASTParser::parseIntLitExpr()
{
    ExprAST *result = new IntLitExprAST(tokenizer.getCurIntLiteral());
    tokenizer.getNextToken(); // consume the number
    return result;
}

/// numberexpr ::= number
ExprAST* ASTParser::parseFloatLitExpr()
{
    ExprAST *result = new FloatLitExprAST(tokenizer.getCurFloatLiteral());
    tokenizer.getNextToken(); // consume the number
    return result;
}

/// stringexpr ::= '"' string '"'
ExprAST* ASTParser::parseStringLitExpr()
{
    ExprAST *result = new StringLitExprAST(tokenizer.getCurIdentifier());
    tokenizer.getNextToken(); // consume the string
    return result;
}

/// boolexpr ::= true
ExprAST* ASTParser::parseBoolLitExpr()
{
    ExprAST *result = new BoolLitExprAST((tokenizer.getCurToken()==tok_true));
    tokenizer.getNextToken(); // consume the bool
    return result;
}

/// parenexpr ::= '(' expression ')'
ExprAST* ASTParser::parseParenExpr()
{
    tokenizer.getNextToken();  // eat the (
    ExprAST *v = parseExpression();
    if (!v) return 0;

    if ((char)tokenizer.getCurToken() != ')')
        return error("expected ')'");
    tokenizer.getNextToken();  // eat )
    return v;
}

/// identifierexpr
///   ::= identifier
///   ::= identifier '(' expression* ')'
ExprAST* ASTParser::parseIdentifierExpr()
{
    std::string idName = tokenizer.getCurIdentifier();

    Token curTok = tokenizer.getNextToken();  // eat identifier

    if ((char)curTok != '(') // Simple variable ref
        return new VariableExprAST(idName);

    // Call
    curTok = tokenizer.getNextToken();  // eat (
    std::vector<ExprAST*> args;
    if ((char)curTok != ')')
    {
        while (1)
        {
            ExprAST *arg = parseExpression();
            if (!arg)
                return 0;
            args.push_back(arg);

            curTok = tokenizer.getCurToken();
            if ((char)curTok == ')')
                break;

            if ((char)curTok != ',')
                return error("Expected ')' or ',' in argument list");
            curTok = tokenizer.getNextToken();
        }
    }

    // Eat the ')'.
    tokenizer.getNextToken();

    return new CallExprAST(idName, args);
}

/// primary
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
ExprAST* ASTParser::parsePrimary()
{
    char tok = (char)tokenizer.getCurToken();
    while (tok == ';') tok = (char)tokenizer.getNextToken();
    switch (tok)
    {
    default:                 return error((std::string("unknown token ")+tok+" when expecting a primary expression").c_str());
    case tok_identifier:     return parseIdentifierExpr();
    case tok_int_literal:    return parseIntLitExpr();
    case tok_float_literal:  return parseFloatLitExpr();
    case tok_string_literal: return parseStringLitExpr();
    case tok_false:
    case tok_true:           return parseBoolLitExpr();
    case tok_if:             return parseIfExpr();
    case '(':                return parseParenExpr();
    case '+':
    case '-':                return parseUnaryExpr();
    case '}':                return new VoidExprAST;
    }
}

/// unary ::= '+' primary
ExprAST* ASTParser::parseUnaryExpr()
{
    int op = tokenizer.getCurToken();
    tokenizer.getNextToken();  // eat op

    // Parse the primary expression after the binary operator.
    ExprAST *rhs = parsePrimary();
    if (!rhs)
        return 0;

    return new UnaryExprAST(op, rhs);
}

/// expression
///   ::= primary binoprhs
///
ExprAST* ASTParser::parseExpression()
{
    ExprAST *lhs = parsePrimary();
    if (!lhs) return 0;

    return parseBinOpRHS(0, lhs);
}

/// binoprhs
///   ::= ('+' primary)*
ExprAST* ASTParser::parseBinOpRHS(int exprPrec, ExprAST *lhs)
{
    // If this is a binop, find its precedence.
    while (1)
    {
        int tokPrec = tokenizer.getCurTokPrecedence();

        // If this is a binop that binds at least as tightly as the current binop,
        // consume it, otherwise we are done.
        if (tokPrec < exprPrec)
            return lhs;

        // Okay, we know this is a binop.
        int binOp = tokenizer.getCurToken();
        tokenizer.getNextToken();  // eat binop

        if (binOp == ';')
            return lhs;

        // Parse the primary expression after the binary operator.
        ExprAST *rhs = parsePrimary();
        if (!rhs)
            return 0;

        // If binOp binds less tightly with RHS than the operator after RHS, let
        // the pending operator take RHS as its LHS.
        int nextPrec = tokenizer.getCurTokPrecedence();
        if (tokPrec < nextPrec)
        {
            rhs = parseBinOpRHS(tokPrec+1, rhs);
            if (rhs == 0) return 0;
        }

        // Merge LHS/RHS.
        lhs = new BinaryExprAST(binOp, lhs, rhs);
    }
}

/// ifexpr ::= 'if' expression 'then' expression 'else' expression
ExprAST* ASTParser::parseIfExpr()
{
    tokenizer.getNextToken();  // eat the if.

    // condition.
    ExprAST *condAST = parseExpression();
    if (!condAST)
    return error("Invalid if expression");

    ExprAST *thenAST = parseBlock();
    if (thenAST == 0)
    return error("Invalid then expression");

    if (tokenizer.getCurToken() != tok_else)
        return new IfExprAST(condAST, thenAST, new VoidExprAST);
    else
        tokenizer.getNextToken();

    ExprAST *elseAST = parseBlock();
    if (!elseAST)
        return error("Invalid else expression");

    return new IfExprAST(condAST, thenAST, elseAST);
}

/// prototype
///   ::= id '(' id* ')'
PrototypeAST* ASTParser::parsePrototype()
{
    Type* retType;

    Token retTok = tokenizer.getCurToken();
    switch (retTok)
    {
        default:    return errorP("Expected return type in function prototype");
        case tok_int:    retType = Type::getInt64Ty(getGlobalContext()); break;
        case tok_float:  retType = Type::getDoubleTy(getGlobalContext()); break;
        case tok_string: retType = Type::getInt8PtrTy(getGlobalContext()); break;
        case tok_bool:   retType = Type::getInt1Ty(getGlobalContext()); break;
        case tok_void:   retType = Type::getVoidTy(getGlobalContext()); break;
    }

    tokenizer.getNextToken();

    if (tokenizer.getCurToken() != tok_identifier)
        return errorP("Expected function name in prototype");

    std::string fnName = tokenizer.getCurIdentifier();
    tokenizer.getNextToken();

    if ((char)tokenizer.getCurToken() != '(')
        return errorP("Expected '(' in prototype");

    // Read the list of argument names.
    std::vector<std::string> argNames;
    std::vector<Type*> argTypes;
    do {
        Type* type;
        Token typeTok = tokenizer.getNextToken();
        if ((char)typeTok == ')')
            break;
        switch (typeTok)
        {
            default:         return errorP("Expected type in function prototype argument list");
            case tok_int:    type = Type::getInt64Ty(getGlobalContext()); break;
            case tok_float:  type = Type::getDoubleTy(getGlobalContext()); break;
            case tok_string: type = Type::getInt8PtrTy(getGlobalContext()); break;
            case tok_bool:   type = Type::getInt1Ty(getGlobalContext()); break;
            case tok_void:   return errorP("Void is not a valid type for a function argument");
        }
        Token nameTok = tokenizer.getNextToken();
        if (nameTok != tok_identifier)
            return errorP("Expected identifier in function prototype argument list");
        argTypes.push_back(type);
        argNames.push_back(tokenizer.getCurIdentifier());
    } while ((char)tokenizer.getNextToken() == ',');

    if ((char)tokenizer.getCurToken() != ')')
        return errorP("Expected ')' in prototype");

    // success.
    tokenizer.getNextToken();  // eat ')'.

    return new PrototypeAST(retType, fnName, argTypes, argNames);
}

/// definition ::= '{ expression* '}'
ExprAST* ASTParser::parseBlock()
{
    if ((char)tokenizer.getCurToken() != '{')
        return error("Expected a { as a start of block");
    else
        tokenizer.getNextToken();

    ExprAST* expr = parseExpression();
    if (!expr)
        return error("Expected expression in block");
    while (1)
    {
        if ((char)tokenizer.getCurToken() == '}')
        {
            tokenizer.getNextToken();
            return expr;
        }

        ExprAST* nextExpr = parseExpression();
        if (!nextExpr)
            return error("Expected expression in block");
        expr = new SequenceExprAST(expr, nextExpr);
    }
}

/// definition ::= prototype expression
FunctionAST* ASTParser::parseDefinition()
{
    PrototypeAST *proto = parsePrototype();
    if (proto == 0)
        return 0;

    if ((char)tokenizer.getCurToken() != '{')
        return errorF("Expected a { after function prototype");
    else
        tokenizer.getNextToken();

    ExprAST* body = parseExpression();
    if (!body)
        return 0;
    while (1)
    {
        if ((char)tokenizer.getCurToken() == '}')
        {
            tokenizer.getNextToken();
            return new FunctionAST(proto, body);
        }

        ExprAST* nextExpr = parseExpression();
        if (!nextExpr)
            return 0;
        body = new SequenceExprAST(body, nextExpr);
    }
    return 0;
}

/// external ::= 'extern' prototype
PrototypeAST* ASTParser::parseExtern()
{
    tokenizer.getNextToken();  // eat extern.
    return parsePrototype();
}

/// toplevelexpr ::= expression
FunctionAST* ASTParser::parseTopLevelExpr()
{
    if (ExprAST *e = parseExpression())
    {
        // Make an anonymous proto.
        PrototypeAST *proto = new PrototypeAST(Type::getVoidTy(getGlobalContext()), "", {}, {});
        return new FunctionAST(proto, e);
    }
    return 0;  
}
