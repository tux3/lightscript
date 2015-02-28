#include "lightscript.h"
#include "mcjithelper.h"

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/TargetSelect.h>

using namespace llvm;
using namespace llvm::legacy;

Lightscript::Lightscript(const std::vector<char> &Script)
    : script{Script},
      module{new Module{"LightScript JIT", getGlobalContext()}},
      FPM{new FunctionPassManager{module}}, tokenizer{script},
      parser{tokenizer}, jit{new MCJITHelper(getGlobalContext())},
      codegen{jit}, optimize{false}
{
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();
}

Lightscript::~Lightscript()
{

}

void Lightscript::handleDefinition()
{
    if (FunctionAST *f = parser.parseDefinition())
    {
        if (Function *lf = codegen.codegen(f))
        {
            fprintf(stderr, "Read function definition:");
            lf->dump();
        }
    }
    else
    {
        // Skip token for error recovery.
        tokenizer.getNextToken();
    }
}

void Lightscript::handleExtern()
{
    if (PrototypeAST *p = parser.parseExtern())
    {
        if (Function *f = codegen.codegen(p))
        {
            fprintf(stderr, "Read extern: ");
            f->dump();
        }
    }
    else
    {
        // Skip token for error recovery.
        tokenizer.getNextToken();
    }
}

void Lightscript::handleTopLevelExpression()
{
    // Evaluate a top-level expression into an anonymous function.
    if (FunctionAST *f = parser.parseTopLevelExpr())
    {
        if (Function *lf = codegen.codegen(f))
        {
            fprintf(stderr, "Read top-level expression:");
            lf->dump();

            // JIT the function, returning a function pointer.
            void *FPtr = jit->getPointerToFunction(lf);

            // Cast it to the right type (takes no arguments, returns a double) so we
            // can call it as a native function.
            double (*fp)() = (double (*)())(intptr_t)FPtr;
            fprintf(stderr, "Evaluated to %f\n", fp());
        }
    }
    else
    {
        // Skip token for error recovery.
        tokenizer.getNextToken();
    }
}

bool Lightscript::compile()
{
    char curTok = (char)tokenizer.getNextToken();
    while (curTok != tok_eof)
    {
        curTok = (char)tokenizer.getCurToken();
        switch (curTok)
        {
            case tok_eof:       break;
            case tok_invalid:   return false;
            case ';':           tokenizer.getNextToken(); break;  // ignore top-level semicolons.
            case tok_int:
            case tok_float:
            case tok_string:
            case tok_bool:
            case tok_void:      handleDefinition(); break;
            case tok_extern:    handleExtern(); break;
            default:            fprintf(stderr, "Code is not allowed outside a function\n"); return false;
        }
    }

    Type* voidTy = Type::getVoidTy(getGlobalContext());
    Type* boolTy = Type::getInt1Ty(getGlobalContext());
    Function* init = jit->getFunction("init");
    if (!init || init->getReturnType() != boolTy
            || init->getArgumentList().size())
    {
        fprintf(stderr, "Script must have an init function of the form 'bool init()'\n");
        return false;
    }
    Function* exit = jit->getFunction("exit");
    if (!exit || exit->getReturnType() != voidTy
            || exit->getArgumentList().size())
    {
        fprintf(stderr, "Script must have an exit function of the form 'void exit()'\n");
        return false;
    }

    return true;
}
