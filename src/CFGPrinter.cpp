#include "CFGPrinter.hpp"

#include "llvm/IR/InstIterator.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

namespace llvm {

/**
 * @brief CFGPrinter implementation
 */
PreservedAnalyses CFGPrinter::run(Function &Func,
                                  FunctionAnalysisManager &FAM) const {
  const auto &LI = FAM.getResult<LoopAnalysis>(Func);

  OS << "Printing analysis 'CFG' for function '"
     << Func.getName() << "':\n";

  for (const auto &L : LI.getLoopsInPreorder()) {
    const auto &HeaderBB = L->getHeader();
    assert(HeaderBB != nullptr && "Empty loop header");

    if (L->isLoopExiting(HeaderBB)) {
      assert(HeaderBB->size() > 2 && "Header Inst size less than 2");
      OS << "for/while loop: ";
      OS << *(----HeaderBB->end());
      OS << "\n";
    } else {
      const auto &LatchBB = L->getLoopLatch();
      assert(LatchBB != nullptr && "Empty latch");

      if (L->isLoopExiting(LatchBB)) {
        assert(LatchBB->size() > 2 && "Latch Inst size less than 2");
        OS << "do-while loop: ";
        OS << *(----LatchBB->end());
        OS << "\n";
      } else {
        OS << "wtf is this loop\n";
        L->print(OS);
      }
    }
  }

  return PreservedAnalyses::all();
}

/**
 * @brief CFGPrinter pass registration callback
 * @note Pass name: "print<cfg>"
 */
PassPluginLibraryInfo getCFGPrinterPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "CFGPrinter", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name != "print<cfg>") {
                    return false;
                  }

                  FPM.addPass(CFGPrinter(errs()));
                  return true;
                });
          }};
}

/**
 * @brief Public entry point for dynamically loaded pass plugin
 */
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getCFGPrinterPluginInfo();
}

} // namespace llvm
