#ifndef MCJITHELPER_H
#define MCJITHELPER_H

#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include <vector>
#include <string>

class MCJITHelper {
public:
  MCJITHelper(llvm::LLVMContext &C) : Context(C), OpenModule(NULL) {}
  ~MCJITHelper();

  llvm::Function *getFunction(const std::string FnName);
  llvm::Module *getModuleForNewFunction();
  void *getPointerToFunction(llvm::Function *F);
  void *getSymbolAddress(const std::string &Name);
  void dump();

private:
  typedef std::vector<llvm::Module *> ModuleVector;
  typedef std::vector<llvm::ExecutionEngine *> EngineVector;

  llvm::LLVMContext &Context;
  llvm::Module *OpenModule;
  ModuleVector Modules;
  EngineVector Engines;
};

class HelpingMemoryManager : public llvm::SectionMemoryManager {
  HelpingMemoryManager(const HelpingMemoryManager &) = delete;
  void operator=(const HelpingMemoryManager &) = delete;

public:
  HelpingMemoryManager(MCJITHelper *Helper) : MasterHelper(Helper) {}
  virtual ~HelpingMemoryManager() {}

  /// This method returns the address of the specified symbol.
  /// Our implementation will attempt to find symbols in other
  /// modules associated with the MCJITHelper to cross link symbols
  /// from one generated module to another.
  virtual uint64_t getSymbolAddress(const std::string &Name) override;

private:
  MCJITHelper *MasterHelper;
};

#endif // MCJITHELPER_H
