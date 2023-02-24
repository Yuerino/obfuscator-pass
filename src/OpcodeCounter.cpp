#include "OpcodeCounter.hpp"

#include "llvm/IR/InstIterator.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

namespace llvm {

static void printOpcodeCounterResult(raw_ostream &,
                                     const ResultOpcodeCounter &);

AnalysisKey OpcodeCounter::Key;

/**
 * @brief OpcodeCounter implementation
 */
OpcodeCounter::Result
OpcodeCounter::run(const Function &Func,
                   const FunctionAnalysisManager &) const {
  OpcodeCounter::Result OpcodeMap;

  for (const_inst_iterator I = inst_begin(Func), E = inst_end(Func); I != E;
       ++I) {
    StringRef Name = I->getOpcodeName();

    if (OpcodeMap.find(Name) == OpcodeMap.end()) {
      OpcodeMap[Name] = 1;
    } else {
      OpcodeMap[Name]++;
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
 * @note Pass name: "print<opcode-counter>"
 */
PassPluginLibraryInfo getOpcodeCounterPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "OpcodeCounter", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name != "print<opcode-counter>") {
                    return false;
                  }

                  FPM.addPass(OpcodeCounterPrinter(errs()));
                  return true;
                });
            PB.registerAnalysisRegistrationCallback(
                [](FunctionAnalysisManager &FAM) {
                  FAM.registerPass([] { return OpcodeCounter(); });
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

/**
 * @brief Helper function to pretty-print the result of opcode counter
 */
static void printOpcodeCounterResult(raw_ostream &OutS,
                                     const ResultOpcodeCounter &OpcodeMap) {
  const char *format = "{0,-20} {1,-10}\n";
  OutS << "=================================================\n";
  OutS << formatv(format, "OPCODE", "#TIMES USED");
  OutS << "-------------------------------------------------\n";
  for (const auto &Inst : OpcodeMap) {
    OutS << formatv(format, Inst.getKey(), Inst.getValue());
  }
  OutS << "-------------------------------------------------\n\n";
}

} // namespace llvm
