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
    return builder.CreateGlobalStringPtr(ast->str, "stringlit");
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

    // Perform type casts if necessary
    Type *intType = builder.getInt64Ty(),
            *floatType = builder.getDoubleTy(),
            *boolType = builder.getInt1Ty(),
            *stringType = builder.getInt8PtrTy();

    if (l->getType() != r->getType())
    {
        if (l->getType() == stringType || r->getType() == stringType)
            return errorV("Invalid binary expression,  no cast from or to 'string' exists");
        else if (l->getType() == floatType)
            r = builder.CreateUIToFP(r, floatType, "casttmp");
        else if (r->getType() == floatType)
            l = builder.CreateUIToFP(l, floatType, "casttmp");
    }

    switch (ast->op)
    {
        case '+':
            if (l->getType() == floatType)
                return builder.CreateFAdd(l, r, "addtmp");
            else if (l->getType() == intType || l->getType() == boolType)
                return builder.CreateAdd(l, r, "addtmp");
            else
                return errorV("Internal error in CodeGen::codegen(BinaryExprAST*)");
        case '-':
            if (l->getType() == floatType)
                return builder.CreateFSub(l, r, "subtmp");
            else if (l->getType() == intType || l->getType() == boolType)
                return builder.CreateSub(l, r, "subtmp");
            else
                return errorV("Internal error in CodeGen::codegen(BinaryExprAST*)");
        case '*':
            if (l->getType() == floatType)
                return builder.CreateFMul(l, r, "multmp");
            else if (l->getType() == intType || l->getType() == boolType)
                return builder.CreateMul(l, r, "multmp");
            else
                return errorV("Internal error in CodeGen::codegen(BinaryExprAST*)");
        case '<':
            if (l->getType() == floatType)
                builder.CreateFCmpULT(l, r, "cmptmp");
            else if (l->getType() == intType || l->getType() == boolType)
                return builder.CreateICmpSLT(l, r, "cmptmp");
            else
                return errorV("Internal error in CodeGen::codegen(BinaryExprAST*)");
        default:
            return errorV("invalid binary operator");
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

    auto it = calleeF->getArgumentList().begin();
    std::vector<Value*> argsV;
    for (unsigned i = 0, e = ast->args.size(); i != e; ++i)
    {

        Value* argV = ast->args[i]->codegen(*this);
        if (argV->getType() != it->getType())
            return errorV(("Incorrect argument type for argument "
                           +std::to_string(i+1)+" in function call of "+ast->callee).c_str());
        else
            it++;
        argsV.push_back(argV);
        if (argsV.back() == 0)
            return 0;
    }

    return builder.CreateCall(calleeF, argsV, "calltmp");
}

Value* CodeGen::codegen(VoidExprAST*)
{
    BasicBlock* target = BasicBlock::Create(getGlobalContext(), "nop", builder.GetInsertBlock()->getParent());
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

Value* CodeGen::codegen(IfExprAST* ast)
{
    Value *condV = ast->condAST->codegen(*this);
    if (condV == 0)
        return 0;

    if (condV->getType() == Type::getDoubleTy(getGlobalContext()))
    {
        // Convert condition to a bool by comparing equal to 0.0.
        condV = builder.CreateFCmpONE(condV,
                        ConstantFP::get(getGlobalContext(), APFloat(0.0)),
                        "ifcond");
    }
    else if (condV->getType() == Type::getInt64Ty(getGlobalContext()))
    {
        // Convert condition to a bool by comparing equal to 0.
        condV = builder.CreateICmpNE(condV,
                        builder.getInt64(0),
                        "ifcond");
    }
    else if (condV->getType() == Type::getInt1Ty(getGlobalContext()))
    {
        // It's already a bool
    }
    else
    {
        return errorV("Expression in if must be an int, float, or bool");
    }

    Function *function = builder.GetInsertBlock()->getParent();

    // Create blocks for the then and else cases.  Insert the 'then' block at the
    // end of the function.
    BasicBlock *thenBB = BasicBlock::Create(getGlobalContext(), "then", function);
    BasicBlock *elseBB = BasicBlock::Create(getGlobalContext(), "else");
    BasicBlock *mergeBB = BasicBlock::Create(getGlobalContext(), "ifcont");

    builder.CreateCondBr(condV, thenBB, elseBB);

    // Emit then value.
    builder.SetInsertPoint(thenBB);

    Value *thenV = ast->thenAST->codegen(*this);
    if (thenV == 0)
        return 0;

    builder.CreateBr(mergeBB);
    // Codegen of 'thenAST' can change the current block, update thenBB for the PHI.
    thenBB = builder.GetInsertBlock();

    // Emit else block.
    function->getBasicBlockList().push_back(elseBB);
    builder.SetInsertPoint(elseBB);

    Value *elseV = ast->elseAST->codegen(*this);
    if (elseV == 0)
        return 0;

    builder.CreateBr(mergeBB);
    // Codegen of 'elseAST' can change the current block, update elseBB for the PHI.
    elseBB = builder.GetInsertBlock();

    if (thenV->getType() != elseV->getType())
        return errorV("The 'then' and 'else' expressions must return the same type");

    // Emit merge block.
    function->getBasicBlockList().push_back(mergeBB);
    builder.SetInsertPoint(mergeBB);
    PHINode *pn = builder.CreatePHI(thenV->getType(),
                                    2, "iftmp");

    pn->addIncoming(thenV, thenBB);
    pn->addIncoming(elseV, elseBB);
    return pn;
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
