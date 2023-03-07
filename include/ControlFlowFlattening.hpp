#pragma once

#include "llvm/IR/PassManager.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/ADT/SmallVector.h"

namespace llvm {

class ControlFlowFlattening : public PassInfoMixin<ControlFlowFlattening> {
public:
  PreservedAnalyses run(Function &Func, FunctionAnalysisManager &FAM);

  static bool isRequired() { return true; }

private:
  BasicBlock *splitEntryBlock(BasicBlock *EntryBlock);

private:
  SmallVector<BasicBlock*, 10> FlattenBB;
};

} // namespace llvm
