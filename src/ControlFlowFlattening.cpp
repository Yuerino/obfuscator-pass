#include "ControlFlowFlattening.hpp"

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Debug.h"
#include <cstdint>
#include <llvm-15/llvm/IR/Instructions.h>
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

  LoopEntry =
      BasicBlock::Create(Func.getContext(), "EntryCase", &Func, EntryBlock);
  LoopEnd = BasicBlock::Create(Func.getContext(), "EndCase", &Func, EntryBlock);

  SwitchInst *SwLoopInst = CreateSwitchLoop(Func, EntryBlock, *RNG);

  // Update switch state in every BB/case
  for (BasicBlock *BB : FlattenBB) {
    Instruction *TermInst = BB->getTerminator();

    if (isa<ReturnInst>(TermInst) || isa<UnreachableInst>(TermInst)) {
      // Skip ret inst
      continue;
    }

    if (isa<ResumeInst>(TermInst) || isa<InvokeInst>(TermInst)) {
      // FIXME: not handling exception related inst for now
      continue;
    }

    if (SwitchInst *SwInst = dyn_cast<SwitchInst>(TermInst)) {
      (void)SwInst; // unused
      // TODO: handle switch inst
      continue;
    }

    if (BranchInst *BrInst = dyn_cast<BranchInst>(TermInst)) {
      if (BrInst->isConditional()) {
        BasicBlock *TrueBB = BrInst->getSuccessor(0);
        BasicBlock *FalseBB = BrInst->getSuccessor(1);

        auto *TrueCaseValue = SwLoopInst->findCaseDest(TrueBB);
        assert(TrueCaseValue != nullptr &&
               "Case to this BB should already be added");
        auto *FalseCaseValue = SwLoopInst->findCaseDest(FalseBB);
        assert(FalseCaseValue != nullptr &&
               "Case to this BB should already be added");

        IRBuilder<> CondBrBuilder(BB);

        auto *SelectInst = CondBrBuilder.CreateSelect(
            BrInst->getCondition(), TrueCaseValue, FalseCaseValue);

        TermInst->eraseFromParent();
        CondBrBuilder.CreateStore(SelectInst, SwitchState);
        CondBrBuilder.CreateBr(LoopEnd);
      }

      else {
        BasicBlock *Successor = BrInst->getSuccessor(0);

        auto *CaseValue = SwLoopInst->findCaseDest(Successor);
        assert(CaseValue != nullptr &&
               "Case to this BB should already be added");

        IRBuilder<> UncondBrBuilder(BB);

        TermInst->eraseFromParent();
        UncondBrBuilder.CreateStore(CaseValue, SwitchState);
        UncondBrBuilder.CreateBr(LoopEnd);
      }
      continue;
    }

    LLVM_DEBUG(dbgs() << "Unhandled basic block, terminated with inst: "
                      << TermInst << "\n");
  }

  dbgs() << Func << "\n";

  return PreservedAnalyses::none();
}

/**
 * @brief Split entry basic block if it is terminated by a conditional control
 flow instruction and add the new splited to FlattenBB otherwise don't
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
      }

      else {
        // Because branch condition can be a constant value
        LLVM_DEBUG(
            dbgs()
            << "Condition of branch inst in entry block is not a condition\n");
      }
    }
  }

  else if (SwitchInst *SwInst =
               dyn_cast<SwitchInst>(EntryBlock->getTerminator())) {
    BasicBlock *SplitedEntry =
        EntryBlock->splitBasicBlockBefore(SwInst, "SplitedEntry");

    FlattenBB.insert(FlattenBB.begin(), EntryBlock);
    EntryBlock = SplitedEntry;
  }

  // FIXME: handle invoke inst

  return EntryBlock;
}

/**
 * @brief Create a switch loop with random state and add cases to all the BB
 * @note This switch should never reach default case
 */
SwitchInst *
ControlFlowFlattening::CreateSwitchLoop(Function &Func, BasicBlock *EntryBlock,
                                        RandomNumberGenerator &RNG) {
  std::uniform_int_distribution<uint32_t> Dist(10);
  IRBuilder<> EntryBuilder(EntryBlock);
  IRBuilder<> LoopEntryBuilder(LoopEntry);
  IRBuilder<> LoopEndBuilder(LoopEnd);

  BasicBlock *SwDefaultBB =
      BasicBlock::Create(Func.getContext(), "DefaultCase", &Func, EntryBlock);
  IRBuilder<> SwDefaultBuilder(SwDefaultBB);
  SwDefaultBuilder.CreateBr(LoopEnd); // Should never reach default case

  // Delete BR terminator to add switch alloca
  EntryBlock->getTerminator()->eraseFromParent();

  // Create and store a random state for switch var
  SwitchState = EntryBuilder.CreateAlloca(EntryBuilder.getInt32Ty(), nullptr,
                                          "SwitchState");
  EntryBuilder.CreateStore(EntryBuilder.getInt32(Dist(RNG)), SwitchState);

  EntryBlock->moveBefore(LoopEntry); // Move it back to the top
  EntryBuilder.CreateBr(LoopEntry);

  LoadInst *SwVar = LoopEntryBuilder.CreateLoad(LoopEntryBuilder.getInt32Ty(),
                                                SwitchState, "SwitchVar");
  LoopEndBuilder.CreateBr(LoopEntry);

  SwitchInst *SwInst = LoopEntryBuilder.CreateSwitch(SwVar, SwDefaultBB);

  // Add switch case to all BB
  for (BasicBlock *BB : FlattenBB) {
    BB->moveBefore(LoopEnd);

    // FIXME: generate and replace unique id for each BB
    auto *CaseValue = dyn_cast<ConstantInt>(ConstantInt::get(
        SwInst->getCondition()->getType(), SwInst->getNumCases() + 1));
    assert(CaseValue != nullptr && "CaseValue can't never be nullptr here");

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
