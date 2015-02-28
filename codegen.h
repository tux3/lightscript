#ifndef CODEGEN_H
#define CODEGEN_H

#include <llvm/IR/Value.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <map>
#include <string>
#include "exprast.h"

class MCJITHelper;

class CodeGen
{
public:
    CodeGen(MCJITHelper* Jit);
    ~CodeGen();

    llvm::Value* codegen(IntLitExprAST *ast);
    llvm::Value* codegen(FloatLitExprAST* ast);
    llvm::Value* codegen(StringLitExprAST* ast);
    llvm::Value* codegen(BoolLitExprAST* ast);
    llvm::Value* codegen(VariableExprAST* ast);
    llvm::Value* codegen(BinaryExprAST *ast);
    llvm::Value* codegen(CallExprAST* ast);
    llvm::Value* codegen(VoidExprAST* ast);
    llvm::Value* codegen(UnaryExprAST* ast);
    llvm::Value* codegen(SequenceExprAST* ast);
    llvm::Function* codegen(PrototypeAST* ast);
    llvm::Function* codegen(FunctionAST* ast);

private:
    llvm::IRBuilder<> builder;
    std::map<std::string, llvm::Value*> symbols;
    MCJITHelper* jit;
};

#endif // CODEGEN_H
