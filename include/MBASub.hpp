#pragma once

#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

namespace llvm {

class MBASub : public PassInfoMixin<MBASub> {
public:
  PreservedAnalyses run(Function &Func, FunctionAnalysisManager &) const;

  static bool isRequired() { return true; }

private:
  bool runOnBasicBlock(BasicBlock &BB) const;
};

} // namespace llvm
