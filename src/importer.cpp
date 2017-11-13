#include "importer.h"
#include "code-generation.h"
#include "type-converter.h"
#include <iostream>
#include <sstream>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/CallingConv.h>

using namespace std;
using namespace llvm;

// OolongFunction

string OolongFunction::getName() const {
    return name;
}

vector<Type*> OolongFunction::getArguments() const {
    return arguments;
}

size_t OolongFunction::getArgumentCount() const {
    return arguments.size();
}

bool OolongFunction::matches(const OolongFunction& other, bool castArguments) const {
    //cout << "Comparing " << to_string(*this) << " to " << to_string(other) << endl;
    if (name != other.name || arguments.size() != other.arguments.size()) {
        return false;
    }
    for (size_t i=0; i<arguments.size(); i++) {
        Type* targetType = arguments[i];
        if (castArguments) {
            if (!TypeConverter::canConvertType(targetType, other.arguments[i], llvmContext)) {
                return false;
            }
        }
        else {
            if (targetType != other.arguments[i]) {
                return false;
            }
        }
    }
    return true;
}

bool OolongFunction::operator==(const OolongFunction& other) const {
    return this->matches(other, false /* find exact match */);
}

bool OolongFunction::operator!=(const OolongFunction& other) const {
    return !(*this == other);
}

bool OolongFunction::operator<(const OolongFunction& other) const {
    //cout << "Comparing " << to_string(*this) << " to " << to_string(other) << endl;
    if (name != other.name) {
        return name < other.name;
    }
    if (arguments.size() != other.arguments.size()) {
        return arguments.size() < other.arguments.size();
    }
    for (size_t i=0; i<arguments.size(); i++) {
        Type* targetType = arguments[i];
        if (targetType != other.arguments[i]) {
            // pointer comparison (relies on consistent type references)
            return arguments[i] < other.arguments[i];
        }
    }
    // equal
    return false;
}

string to_string(const OolongFunction& function) {
    stringstream str;
    str << function.getName();
    str << "(";
    auto arguments = function.getArguments();
    for (size_t i=0; i<arguments.size(); i++) {
        if (i != 0) {
            str << ", ";
        }
        str << TypeConverter::getTypeName(arguments[i]);
    }
    str << ")";
    return str.str();
}

// Importer

static void createIoFunctions(Importer* importer, CodeGenerationContext* context);

void Importer::declareFunction(const OolongFunction& function, Function* functionReference) {
    importedFunctions[function] = functionReference;

    //cout << "Declared " << to_string(function) << endl;
}

void Importer::declareExternalFunction(Type* returnType, const OolongFunction& function, const char* externalName, CodeGenerationContext* context) {
    FunctionType* functionType = FunctionType::get(returnType, function.getArguments(), true);
    Function* functionReference = Function::Create(functionType, Function::ExternalLinkage, Twine(externalName), context->getModule());
    functionReference->setCallingConv(CallingConv::C);

    importedFunctions[function] = functionReference;

    //cout << "Declared " << to_string(function) << endl;
}

bool Importer::importPackage(const string& package, CodeGenerationContext* context) {
    if (package == "io") {
        createIoFunctions(this, context);
        return true;
    } else {
        cerr << "Unable to find package: " << package << endl;
        return false;
    }
}

Function* Importer::findFunction(const OolongFunction& function) const{
    auto it = importedFunctions.find(function);
    if (it != importedFunctions.end()) {
        return it->second;
    }
    else {
        // no exact match, try to convert arguments to find a match
        for (auto pair : importedFunctions) {
            OolongFunction candidate = pair.first;
            if (function.matches(candidate, true /* allow casting */)) {
                return pair.second;
            }
        }
        // unable to find match
        return nullptr;
    }
}

static void createIoFunctions(Importer* importer, CodeGenerationContext* context) {
    LLVMContext& llvmContext = context->getLLVMContext();
    Type* voidType = Type::getVoidTy(llvmContext);

    // print String
    vector<Type*> arguments;
    arguments.push_back(Type::getInt8PtrTy(llvmContext));
    OolongFunction function("io.print", arguments, &llvmContext);
    importer->declareExternalFunction(voidType, function, "___io___print_String", context);

    // print Integer
    arguments.clear();
    arguments.push_back(Type::getInt64Ty(llvmContext));
    function = OolongFunction("io.print", arguments, &llvmContext);
    importer->declareExternalFunction(voidType, function, "___io___print_Integer", context);

    //// print Double
    arguments.clear();
    arguments.push_back(Type::getDoubleTy(llvmContext));
    function = OolongFunction("io.print", arguments, &llvmContext);
    importer->declareExternalFunction(voidType, function, "___io___print_Double", context);

    //// print Boolean
    arguments.clear();
    arguments.push_back(Type::getInt1Ty(llvmContext));
    function = OolongFunction("io.print", arguments, &llvmContext);
    importer->declareExternalFunction(voidType, function, "___io___print_Boolean", context);
}

