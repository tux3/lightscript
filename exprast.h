#ifndef EXPRAST_H
#define EXPRAST_H

#include <llvm/IR/Value.h>

#include <vector>
#include <string>

class Tokenizer;
class CodeGen;

/// ExprAST - Base class for all expression nodes.
class ExprAST
{
public:
    ExprAST();
    virtual ~ExprAST();
    virtual llvm::Value* codegen(CodeGen& gen) = 0; ///< Uses the vtable to call the right CodeGen::codegen() overload

    friend class CodeGen;
};

/// IntLitExprAST - Expression class for integer numeric literals like 123
class IntLitExprAST : public ExprAST {
    int64_t val;
public:
    IntLitExprAST(int64_t Val) : val(Val) {}

    virtual llvm::Value* codegen(CodeGen& gen);
    friend class CodeGen;
};

/// FloatLitExprAST - Expression class for numeric literals like 12.50
class FloatLitExprAST : public ExprAST {
    double val;
public:
    FloatLitExprAST(double Val) : val(Val) {}

    virtual llvm::Value* codegen(CodeGen& gen);
    friend class CodeGen;
};

/// StringLitExprAST - Expression class for string literals like "abc"
class StringLitExprAST : public ExprAST {
    std::string str;
public:
    StringLitExprAST(const std::string& Str) : str(Str) {}

    virtual llvm::Value* codegen(CodeGen& gen);
    friend class CodeGen;
};

/// BoolLitExprAST - Expression class for boolean literals (true and false)
class BoolLitExprAST : public ExprAST {
    bool val;
public:
    BoolLitExprAST(bool Val) : val(Val) {}

    virtual llvm::Value* codegen(CodeGen& gen);
    friend class CodeGen;
};

/// VariableExprAST - Expression class for referencing a variable, like "a".
class VariableExprAST : public ExprAST {
    std::string name;
public:
    VariableExprAST(const std::string &Name) : name(Name) {}

    virtual llvm::Value* codegen(CodeGen& gen);
    friend class CodeGen;
};

/// UnaryExprAST - Expression class for a void value
class VoidExprAST : public ExprAST {
public:
    VoidExprAST() {}

    virtual llvm::Value* codegen(CodeGen& gen);
    friend class CodeGen;
};

/// UnaryExprAST - Expression class for a unary operator.
class UnaryExprAST : public ExprAST {
    char op;
    ExprAST *rhs;
public:
    UnaryExprAST(char Op, ExprAST *RHS)
    : op(Op), rhs(RHS) {}

    virtual llvm::Value* codegen(CodeGen& gen);
    friend class CodeGen;
};

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST {
    char op;
    ExprAST *lhs, *rhs;
public:
    BinaryExprAST(char Op, ExprAST *LHS, ExprAST *RHS)
      : op(Op), lhs(LHS), rhs(RHS) {}

    virtual llvm::Value* codegen(CodeGen& gen);
    friend class CodeGen;
};

/// SequenceExprAST - Expression class for an operator that computes both and returns the rhs
class SequenceExprAST : public ExprAST {
    ExprAST *lhs, *rhs;
public:
    SequenceExprAST(ExprAST *LHS, ExprAST *RHS)
      : lhs(LHS), rhs(RHS) {}

    virtual llvm::Value* codegen(CodeGen& gen);
    friend class CodeGen;
};

/// CallExprAST - Expression class for function calls.
class CallExprAST : public ExprAST {
    std::string callee;
    std::vector<ExprAST*> args;
public:
    CallExprAST(const std::string &Callee, std::vector<ExprAST*> &Args)
      : callee(Callee), args(Args) {}

    virtual llvm::Value* codegen(CodeGen& gen);
    friend class CodeGen;
};

/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes).
class PrototypeAST {
    llvm::Type* retType;
    std::string name;
    std::vector<llvm::Type*> argTypes;
    std::vector<std::string> argNames;
public:
    PrototypeAST(llvm::Type* RetType, const std::string &Name,
                 const std::vector<llvm::Type*>& ArgTypes,
                 const std::vector<std::string> &ArgNames)
      : retType{RetType}, name{Name}, argTypes{ArgTypes}, argNames{ArgNames} {}

    friend class CodeGen;
};

/// FunctionAST - This class represents a function definition itself.
class FunctionAST {
    PrototypeAST *proto;
    ExprAST *body;
public:
    FunctionAST(PrototypeAST *Proto, ExprAST *Body)
      : proto(Proto), body(Body) {}

    friend class CodeGen;
};

/// Creates AST nodes from a tokenizer
class ASTParser
{
public:
    ASTParser(Tokenizer& Tokenizer);

    ExprAST* parseIntLitExpr();
    ExprAST* parseFloatLitExpr();
    ExprAST* parseStringLitExpr();
    ExprAST* parseBoolLitExpr();
    ExprAST* parseParenExpr();
    ExprAST* parseIdentifierExpr();
    ExprAST* parseUnaryExpr();
    ExprAST* parsePrimary();
    ExprAST* parseExpression();
    ExprAST* parseBinOpRHS(int exprPrec, ExprAST *lhs);
    PrototypeAST* parsePrototype();
    FunctionAST* parseDefinition();
    PrototypeAST* parseExtern();
    FunctionAST* parseTopLevelExpr();

private:
    ExprAST *error(const char *str);
    PrototypeAST *errorP(const char *str);
    FunctionAST *errorF(const char *str);

private:
    Tokenizer& tokenizer;
};

#endif // EXPRAST_H
