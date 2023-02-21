#include "OpcodeCounter.hpp"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

namespace llvm {

/**
 * @brief Helper function to pretty-print the result of opcode counter
 */
static void printOpcodeCounterResult(raw_ostream &OutS,
                                     const ResultOpcodeCounter &OpcodeMap) {
  OutS << "=================================================\n";
  const char *str1 = "OPCODE";
  const char *str2 = "#TIMES USED";
  OutS << format("%-20s %-10s\n", str1, str2);
  OutS << "-------------------------------------------------\n";
  for (const auto &Inst : OpcodeMap) {
    OutS << format("%-20s %-10lu\n", Inst.first().str().c_str(), Inst.second);
  }
  OutS << "-------------------------------------------------\n\n";
}

AnalysisKey OpcodeCounter::Key;

/**
 * @brief OpcodeCounter implementation
 */
OpcodeCounter::Result
OpcodeCounter::run(const Function &Func,
                   const FunctionAnalysisManager &) const {
  OpcodeCounter::Result OpcodeMap;

  for (const auto &BB : Func) {
    for (const auto &Inst : BB) {
      StringRef Name = Inst.getOpcodeName();

      if (OpcodeMap.find(Name) == OpcodeMap.end()) {
        OpcodeMap[Inst.getOpcodeName()] = 1;
      } else {
        OpcodeMap[Inst.getOpcodeName()]++;
      }
    }
  }

  return OpcodeMap;
}

/**
 * @brief OpcodeCounterPrinter implementation
 */
PreservedAnalyses
OpcodeCounterPrinter::run(Function &Func, FunctionAnalysisManager &FAM) const {
  const auto &OpcodeMap = FAM.getResult<OpcodeCounter>(Func);
  OS << "Printing analysis 'OpcodeCounter Pass' for function '"
     << Func.getName() << "':\n";
  printOpcodeCounterResult(OS, OpcodeMap);
  return PreservedAnalyses::all();
}

/**
 * @brief OpcodeCounter and OpcodeCounterPrinter pass registration callback
 * @note Pass name: "opcode-counter"
 */
PassPluginLibraryInfo getOpcodeCounterPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "OpcodeCounter", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name != "opcode-counter") {
                    return false;
                  }

                  FPM.addPass(OpcodeCounterPrinter(errs()));
                  return true;
                });
            PB.registerAnalysisRegistrationCallback(
                [](FunctionAnalysisManager &FAM) {
                  FAM.registerPass([&] { return OpcodeCounter(); });
                });
          }};
}

/**
 * @brief Public entry point for dynamically loaded pass plugin
 */
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getOpcodeCounterPluginInfo();
}

} // namespace llvm
