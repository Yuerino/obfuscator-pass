#pragma once

#include "llvm/ADT/StringMap.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

namespace llvm {

using ResultOpcodeCounter = StringMap<size_t>;

class OpcodeCounter : public AnalysisInfoMixin<OpcodeCounter> {
public:
  using Result = ResultOpcodeCounter;

  Result run(const Function &Func, const FunctionAnalysisManager &) const;

  static bool isRequired() { return true; }

private:
  static AnalysisKey Key;
  friend struct AnalysisInfoMixin<OpcodeCounter>;
};

class OpcodeCounterPrinter : public PassInfoMixin<OpcodeCounterPrinter> {
public:
  explicit OpcodeCounterPrinter(raw_ostream &OutS) : OS(OutS) {}

  PreservedAnalyses run(Function &Func, FunctionAnalysisManager &FAM) const;

  static bool isRequired() { return true; }

private:
  raw_ostream &OS;
};

} // namespace llvm
