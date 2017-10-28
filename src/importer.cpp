#include "importer.h"
#include <iostream>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/CallingConv.h>

using namespace std;
using namespace llvm;

void createIoFunctions(CodeGenerationContext& context);

bool Importer::importPackage(const string& package, CodeGenerationContext& context) const {
    if (package == "io") {
        createIoFunctions(context);
        return true;
    } else {
        cerr << "Unable to find package: " << package << endl;
        return false;
    }
}

void createIoFunctions(CodeGenerationContext& context) {
    // print_string
    vector<Type*> printStringTypes;
    printStringTypes.push_back(Type::getInt8PtrTy(context.getLLVMContext())); // char*
    FunctionType* printStringType = FunctionType::get(Type::getVoidTy(context.getLLVMContext()), printStringTypes, true);
    Function *printStringFunction = Function::Create(printStringType, Function::ExternalLinkage, Twine("io___print"), context.getModule());
    printStringFunction->setCallingConv(CallingConv::C);
}

