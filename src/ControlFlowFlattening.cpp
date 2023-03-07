#include "ControlFlowFlattening.hpp"

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/RandomNumberGenerator.h"
#include <memory>

#define DEBUG_TYPE "cff"

namespace llvm {

PreservedAnalyses ControlFlowFlattening::run(Function &Func,
                                             FunctionAnalysisManager &) {
  std::unique_ptr<RandomNumberGenerator> RNG =
      Func.getParent()->createRNG(Func.getName());
  LLVM_DEBUG(dbgs() << "Running CFF on " << Func.getName() << "\n");

  BasicBlock *EntryBlock = &Func.getEntryBlock();

  for (BasicBlock &BB : Func) {
    if (&BB == EntryBlock) {
      continue;
    }

    // FIXME: not handling function with exception and invoke for now
    if (BB.isLandingPad() || isa<InvokeInst>(BB.getTerminator())) {
      return PreservedAnalyses::all();
    }

    FlattenBB.push_back(&BB);
  }

  if (FlattenBB.size() <= 1) {
    LLVM_DEBUG(dbgs() << Func.getName() << " is too small to be flattened\n");
    return PreservedAnalyses::all();
  }

  EntryBlock = splitEntryBlock(EntryBlock);

  dbgs() << Func;

  return PreservedAnalyses::none();
}

BasicBlock *ControlFlowFlattening::splitEntryBlock(BasicBlock *EntryBlock) {
  if (BranchInst *BrInst = dyn_cast<BranchInst>(EntryBlock->getTerminator())) {
    if (BrInst->isConditional()) {
      Value *Condition = BrInst->getCondition();

      if (Instruction *CondInst = dyn_cast<Instruction>(Condition)) {
        BasicBlock *SplitedEntry =
            EntryBlock->splitBasicBlockBefore(CondInst, "SplitedEntry");
        FlattenBB.insert(FlattenBB.begin(), SplitedEntry);
        EntryBlock = SplitedEntry;
      } else {
        LLVM_DEBUG(
            dbgs()
            << "Condition of branch inst in entry block is not a condition\n");
      }
    }
  } else if (SwitchInst *SwInst =
                 dyn_cast<SwitchInst>(EntryBlock->getTerminator())) {
    BasicBlock *SplitedEntry =
        EntryBlock->splitBasicBlockBefore(SwInst, "SplitedEntry");
    FlattenBB.insert(FlattenBB.begin(), SplitedEntry);
    EntryBlock = SplitedEntry;
  }

  return EntryBlock;
}

PassPluginLibraryInfo getControlFlowFlatteningPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "ControlFlowFlattening", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name != "cff") {
                    return false;
                  }

                  FPM.addPass(ControlFlowFlattening());
                  return true;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getControlFlowFlatteningPluginInfo();
}

} // namespace llvm
