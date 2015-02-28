#ifndef LIGHTSCRIPT_H
#define LIGHTSCRIPT_H

#include <vector>
#include "tokenizer.h"
#include "exprast.h"
#include "codegen.h"

namespace llvm{
class Module;
class ExecutionEngine;
namespace legacy{class FunctionPassManager;}
}
class MCJITHelper;

/// Compiles, runs, and interracts with a single script.
class Lightscript
{
public:
    Lightscript(const std::vector<char>& script);
    ~Lightscript();

    bool compile();

private:
    void handleExtern();
    void handleDefinition();
    void handleTopLevelExpression();

private:
    const std::vector<char> script;
    llvm::Module *module;
    llvm::legacy::FunctionPassManager* FPM;
    Tokenizer tokenizer;
    ASTParser parser;
    MCJITHelper* jit;
    CodeGen codegen;
    bool optimize;
};

#endif // LIGHTSCRIPT_H
