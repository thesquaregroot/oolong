#include "common.h"
#include "llvm/IR/Function.h"
#include <string>
#include <iostream>

using namespace std;
using namespace llvm;

extern const char* currentParseFile;

Value* warning(CodeGenerationContext& context, const string& warningMessage) {
    cerr << "warning: In file " << currentParseFile << ", function " << context.currentFunction()->getName().str() << ": " << warningMessage << endl;
    return nullptr;
}

Value* error(CodeGenerationContext& context, const string& errorMessage) {
    const string fullError =  "In file " + string(currentParseFile) +  ", function " + context.currentFunction()->getName().str() + ": " + errorMessage;
    context.getLLVMContext().emitError(fullError);
    return nullptr;
}

