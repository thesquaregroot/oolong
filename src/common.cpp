#include "common.h"
#include "llvm/IR/Function.h"
#include <string>
#include <iostream>

using namespace std;
using namespace llvm;

extern const char* currentParseFile;

static string getFunctionContext(CodeGenerationContext& context) {
    string functionContext = "";
    if (context.currentFunction() != nullptr) {
        functionContext = ", function " + context.currentFunction()->getName().str();
    }
    return functionContext;
}

Value* warning(CodeGenerationContext& context, const string& warningMessage) {
    cerr << "warning: In file " << currentParseFile << getFunctionContext(context) << ": " << warningMessage << endl;
    return nullptr;
}

Value* error(CodeGenerationContext& context, const string& errorMessage) {
    const string fullError =  "In file " + string(currentParseFile) + getFunctionContext(context) + ": " + errorMessage;
    context.getLLVMContext().emitError(fullError);
    return nullptr;
}

