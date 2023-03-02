#include "MBASub.hpp"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Value.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#define DEBUG_TYPE "mba-sub"

namespace llvm {

/**
 * @brief MBA Sub Implementation
 */
bool MBASub::runOnBasicBlock(BasicBlock &BB) const {
  bool Changed = false;

  for (auto Inst = BB.begin(), IE = BB.end(); Inst != IE; ++Inst) {
    auto *BinOp = dyn_cast<BinaryOperator>(Inst);
    if (BinOp == nullptr) {
      continue;
    }

    // Only handle sub instruction
    if (BinOp->getOpcode() != Instruction::Sub) {
      continue;
    }

    // Only handle integer types.
    if (!BinOp->getType()->isIntegerTy()) {
      continue;
    }

    IRBuilder<> Builder(BinOp);

    // (a + ~b)
    Value *FirstHalf = Builder.CreateAdd(
        BinOp->getOperand(0), Builder.CreateNot(BinOp->getOperand(1)));
    // (a + ~b) + 1
    Instruction *NewInst = BinaryOperator::CreateAdd(
        FirstHalf, ConstantInt::get(BinOp->getType(), 1));

    LLVM_DEBUG(dbgs() << *BinOp << " -> " << *NewInst << "\n");

    ReplaceInstWithInst(BB.getInstList(), Inst, NewInst);
    Changed = true;
  }

  return Changed;
}

PreservedAnalyses MBASub::run(Function &Func, FunctionAnalysisManager &) const {
  bool Changed = false;

  for (auto &BB : Func) {
    Changed |= runOnBasicBlock(BB);
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

PassPluginLibraryInfo getMBASubPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "MBASub", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name != "mba-sub") {
                    return false;
                  }

                  FPM.addPass(MBASub());
                  return true;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getMBASubPluginInfo();
}

} // namespace llvm
