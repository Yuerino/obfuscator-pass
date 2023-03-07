#include "ControlFlowFlattening.hpp"

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Debug.h"
#include <cstdint>
#include <memory>
#include <random>

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

  SwitchInst *SwInst = CreateSwitchLoop(Func, EntryBlock, *RNG);
  (void)SwInst; // FIXME: unused;

  // TODO: Update all BB block flow accordingly

  return PreservedAnalyses::none();
}

/**
 * @brief Split entry basic block if it is conditional control flow and add it
 * to FlattenBB
 * @return BasicBlock* New entry basic block after splited otherwise same
 * EntryBlock
 */
BasicBlock *ControlFlowFlattening::splitEntryBlock(BasicBlock *EntryBlock) {
  if (BranchInst *BrInst = dyn_cast<BranchInst>(EntryBlock->getTerminator())) {
    if (BrInst->isConditional()) {
      Value *Condition = BrInst->getCondition();

      if (Instruction *CondInst = dyn_cast<Instruction>(Condition)) {
        BasicBlock *SplitedEntry =
            EntryBlock->splitBasicBlockBefore(CondInst, "SplitedEntry");
        FlattenBB.insert(FlattenBB.begin(), EntryBlock);
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
    FlattenBB.insert(FlattenBB.begin(), EntryBlock);
    EntryBlock = SplitedEntry;
  }

  return EntryBlock;
}

/**
 * @brief Create a switch loop with random state and cases to all the BB
 * @note This switch should never reach default case
 */
SwitchInst *
ControlFlowFlattening::CreateSwitchLoop(Function &Func, BasicBlock *EntryBlock,
                                        RandomNumberGenerator &RNG) {
  std::uniform_int_distribution<uint32_t> Dist(10);
  IRBuilder<> EntryBuilder(EntryBlock);

  // Delete BR terminator to add switch alloca
  EntryBlock->getTerminator()->eraseFromParent();

  // Create and store a random state for switch var
  AllocaInst *StateVar =
      EntryBuilder.CreateAlloca(EntryBuilder.getInt32Ty(), nullptr, "StateVar");
  EntryBuilder.CreateStore(EntryBuilder.getInt32(Dist(RNG)), StateVar);

  BasicBlock *LoopEntryBB =
      BasicBlock::Create(Func.getContext(), "EntryCase", &Func, EntryBlock);
  BasicBlock *LoopEndBB =
      BasicBlock::Create(Func.getContext(), "EndCase", &Func, EntryBlock);
  BasicBlock *SwDefaultBB =
      BasicBlock::Create(Func.getContext(), "DefaultCase", &Func, EntryBlock);

  IRBuilder<> LoopEntryBuilder(LoopEntryBB);
  IRBuilder<> LoopEndBuilder(LoopEndBB);
  IRBuilder<> SwDefaultBuilder(SwDefaultBB);

  EntryBlock->moveBefore(LoopEntryBB); // Move it back to the top
  EntryBuilder.CreateBr(LoopEntryBB);

  LoadInst *SwVar = LoopEntryBuilder.CreateLoad(LoopEntryBuilder.getInt32Ty(),
                                                StateVar, "SwitchVar");
  LoopEndBuilder.CreateBr(LoopEntryBB);
  SwDefaultBuilder.CreateBr(LoopEndBB);

  SwitchInst *SwInst = LoopEntryBuilder.CreateSwitch(SwVar, SwDefaultBB);

  // Add switch case to all BB
  for (BasicBlock *BB : FlattenBB) {
    BB->moveBefore(LoopEndBB);

    // FIXME: generate and replace unique id for each BB
    auto *CaseValue = dyn_cast<ConstantInt>(ConstantInt::get(
        SwInst->getCondition()->getType(), SwInst->getNumCases() + 1));
    assert(CaseValue != nullptr && "How can this be null");

    SwInst->addCase(CaseValue, BB);
  }

  return SwInst;
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
