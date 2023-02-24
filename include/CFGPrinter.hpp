#pragma once

#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

namespace llvm {

class CFGPrinter : public PassInfoMixin<CFGPrinter> {
public:
  explicit CFGPrinter(raw_ostream &OutS) : OS(OutS) {}

  PreservedAnalyses run(Function &Func, FunctionAnalysisManager &FAM) const;

  static bool isRequired() { return true; }

private:
  raw_ostream &OS;
};

} // namespace llvm
