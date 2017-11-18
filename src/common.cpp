#include "common.h"
#include "llvm/IR/Function.h"
#include <string>
#include <iostream>

using namespace std;
using namespace llvm;

extern const char* currentParseFile;

// general string function, replace all occurrences of a substring with another
void replaceAll(string& value, const string& substring, const string& replacement) {
    size_t index = value.find(substring);
    while (index != string::npos) {
        // replace \" sequence with just "
        value.replace(index, substring.length(), replacement);
        // re-search starting at end of replacement
        index = value.find("\\\"", index + replacement.length() - 1);
    }
}

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

