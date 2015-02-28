#include "codegen.h"
#include "mcjithelper.h"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Verifier.h>

using namespace llvm;

static ExprAST *error(const char *str) { fprintf(stderr, "Error: %s\n", str);return 0;}
static Value* errorV(const char *str) { error(str); return 0; }
static Function* errorF(const char *str) { error(str); return 0; }

CodeGen::CodeGen(MCJITHelper *Jit)
    : builder{getGlobalContext()}, jit{Jit}
{
}

CodeGen::~CodeGen()
{
}

Value* CodeGen::codegen(IntLitExprAST* ast)
{
    return ConstantInt::get(getGlobalContext(), APInt(sizeof(ast->val)*8, ast->val, true));
}

Value* CodeGen::codegen(FloatLitExprAST *ast)
{
    return ConstantFP::get(getGlobalContext(), APFloat(ast->val));
}

Value* CodeGen::codegen(BoolLitExprAST *ast)
{
    if (ast->val)
        return ConstantInt::getTrue(getGlobalContext());
    else
        return ConstantInt::getFalse(getGlobalContext());
}

Value* CodeGen::codegen(StringLitExprAST *ast)
{
    return ConstantDataArray::getString(getGlobalContext(), ast->str, true);
}
Value* CodeGen::codegen(VariableExprAST* ast)
{
    // Look this variable up in the function.
    Value *v = symbols[ast->name];
    return v ? v : errorV(("Unknown variable name: "+ast->name).c_str());
}

Value* CodeGen::codegen(BinaryExprAST* ast)
{
    Value *l = ast->lhs->codegen(*this);
    Value *r = ast->rhs->codegen(*this);
    if (l == 0 || r == 0)
        return 0;

    switch (ast->op)
    {
        case '+': return builder.CreateFAdd(l, r, "addtmp");
        case '-': return builder.CreateFSub(l, r, "subtmp");
        case '*': return builder.CreateFMul(l, r, "multmp");
        case '<':
            l = builder.CreateFCmpULT(l, r, "cmptmp");
            // Convert bool 0/1 to double 0.0 or 1.0
            return builder.CreateUIToFP(l, Type::getDoubleTy(getGlobalContext()),
                                        "booltmp");
        default: return errorV("invalid binary operator");
    }
}

Value* CodeGen::codegen(CallExprAST *ast)
{
    // Look up the name in the global module table.
    Function *calleeF = jit->getFunction(ast->callee);
    if (calleeF == 0)
        return errorV(("Unknown function referenced: "+ast->callee).c_str());

    // If argument mismatch error.
    if (calleeF->arg_size() != ast->args.size())
        return errorV(("Incorrect number of arguments passed to "+ast->callee).c_str());

    std::vector<Value*> argsV;
    for (unsigned i = 0, e = ast->args.size(); i != e; ++i)
    {
        argsV.push_back(ast->args[i]->codegen(*this));
        if (argsV.back() == 0)
            return 0;
    }

    return builder.CreateCall(calleeF, argsV, "calltmp");
}

Value* CodeGen::codegen(VoidExprAST*)
{
    BasicBlock* target = BasicBlock::Create(getGlobalContext(), "then", builder.GetInsertBlock()->getParent());
    Value* r = builder.CreateBr(target);
    builder.SetInsertPoint(target);
    return r;
}

Value* CodeGen::codegen(UnaryExprAST* ast)
{
    Value *r = ast->rhs->codegen(*this);
    if (r == 0)
        return 0;

    switch (ast->op)
    {
        case '+': return r;
        case '-': return builder.CreateNeg(r, "negtmp");
        default: return errorV("invalid unary operator");
    }
}

Value* CodeGen::codegen(SequenceExprAST* ast)
{
    Value *l = ast->lhs->codegen(*this);
    Value *r = ast->rhs->codegen(*this);
    if (l == 0 || r == 0)
        return 0;

    return r;
}

Function* CodeGen::codegen(PrototypeAST* ast)
{
    // Make the function type:  double(double,double) etc.
    FunctionType *ft = FunctionType::get(ast->retType, ast->argTypes, false);

    Module *m = jit->getModuleForNewFunction();

    Function *f = Function::Create(ft, Function::ExternalLinkage, ast->name, m);

    // If F conflicted, there was already something named 'Name'.  If it has a
    // body, don't allow redefinition or reextern.
    if (f->getName() != ast->name)
    {
        // Delete the one we just made and get the existing one.
        f->eraseFromParent();
        f = jit->getFunction(ast->name);

        // If F already has a body, reject this.
        if (!f->empty())
        {
            errorF("redefinition of function");
            return 0;
        }

        // If F took a different number of args, reject.
        if (f->arg_size() != ast->argNames.size())
        {
            errorF("redefinition of function with different # args");
            return 0;
        }
    }

    // Set names for all arguments.
    unsigned idx = 0;
    for (Function::arg_iterator ai = f->arg_begin(); idx != ast->argNames.size(); ++ai, ++idx)
    {
        ai->setName(ast->argNames[idx]);

        // Add arguments to variable symbol table.
        symbols[ast->argNames[idx]] = ai;
    }

    return f;
}

Function* CodeGen::codegen(FunctionAST* ast)
{
    symbols.clear();

    Function *function = codegen(ast->proto);
    if (function == 0)
        return 0;

    // Create a new basic block to start insertion into.
    BasicBlock *bb = BasicBlock::Create(getGlobalContext(), "entry", function);
    builder.SetInsertPoint(bb);

    if (Value *retVal = ast->body->codegen(*this))
    {
        // Finish off the function.
        if (ast->proto->retType->isVoidTy())
        {
            if (retVal && retVal->getType() != Type::getVoidTy(getGlobalContext()))
                fprintf(stderr, "Warning: Non-void return value in void function '%s'\n", ast->proto->name.c_str());
            builder.CreateRetVoid();
        }
        else
        {
            if (retVal->getType() != ast->proto->retType)
                return errorF(("Return value type doesn't match"
                              " function prototype in '"
                              +ast->proto->name+"'\n").c_str());
            builder.CreateRet(retVal);
        }

        // Validate the generated code, checking for consistency.
        verifyFunction(*function);

        return function;
    }

    // Error reading body, remove function.
    function->eraseFromParent();
    return 0;
}
