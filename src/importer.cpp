#include "importer.h"
#include "code-generation.h"
#include "common.h"
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

static const string EXTERNAL_FUNCTION_PREFIX = "___";
static const string EXTERNAL_FUNCTION_PACKAGE_SEPARATOR = "___";
static const string EXTERNAL_FUNCTION_ARGUMENT_SEPARATOR = "_";

static void createDefaultFunctions(Importer* importer, CodeGenerationContext* context);
static void createIoFunctions(Importer* importer, CodeGenerationContext* context);

void Importer::declareFunction(const OolongFunction& function, Function* functionReference) {
    importedFunctions[function] = functionReference;

    //cout << "Declared " << to_string(function) << endl;
}

void Importer::declareExternalFunction(Type* returnType, const OolongFunction& function, CodeGenerationContext* context) {
    string externalName = function.getName();
    // replace dots
    replaceAll(externalName, ".", EXTERNAL_FUNCTION_PACKAGE_SEPARATOR);
    // add prefix
    externalName = EXTERNAL_FUNCTION_PREFIX + externalName;
    // add arguments to name
    for (Type* argumentType : function.getArguments()) {
        externalName += EXTERNAL_FUNCTION_ARGUMENT_SEPARATOR + TypeConverter::getTypeName(argumentType);
    }

    FunctionType* functionType = FunctionType::get(returnType, function.getArguments(), true);
    Function* functionReference = Function::Create(functionType, Function::ExternalLinkage, Twine(externalName), context->getModule());
    functionReference->setCallingConv(CallingConv::C);

    importedFunctions[function] = functionReference;

    //cout << "Declared " << to_string(function) << endl;
}

bool Importer::importPackage(const string& package, CodeGenerationContext* context) {
    if (package == "") {
        // should be auto-imported
        createDefaultFunctions(this, context);
    }
    else if (package == "io") {
        createIoFunctions(this, context);
    } else {
        cerr << "Unable to find package: " << package << endl;
        return false;
    }
    return true;
}

Function* Importer::findFunction(const OolongFunction& function) const{
    return this->findFunction(function, false /* inexact match */);
}

Function* Importer::findFunction(const OolongFunction& function, bool exactMatch) const{
    auto it = importedFunctions.find(function);
    if (it != importedFunctions.end()) {
        return it->second;
    }
    else {
        // no exact match
        if (!exactMatch) {
            // try to convert arguments to find a match
            for (auto pair : importedFunctions) {
                OolongFunction candidate = pair.first;
                if (function.matches(candidate, true /* allow casting */)) {
                    return pair.second;
                }
            }
        }
        // unable to find match
        return nullptr;
    }
}

static void createDefaultFunctions(Importer* importer, CodeGenerationContext* context) {
    LLVMContext& llvmContext = context->getLLVMContext();

    Type* booleanType = TypeConverter::getBooleanType(llvmContext);
    Type* integerType = TypeConverter::getIntegerType(llvmContext);
    Type* doubleType = TypeConverter::getDoubleType(llvmContext);
    Type* stringType = TypeConverter::getStringType(llvmContext);

    // Boolean functions
    vector<Type*> arguments;
    arguments.push_back(booleanType);
    OolongFunction function("toInteger", arguments, &llvmContext);
    importer->declareExternalFunction(booleanType, function, context);
    function = OolongFunction("toDouble", arguments, &llvmContext);
    importer->declareExternalFunction(doubleType, function, context);
    function = OolongFunction("toString", arguments, &llvmContext);
    importer->declareExternalFunction(stringType, function, context);

    // Integer functions
    arguments.clear();
    arguments.push_back(integerType);
    function = OolongFunction("toBoolean", arguments, &llvmContext);
    importer->declareExternalFunction(booleanType, function, context);
    function = OolongFunction("toDouble", arguments, &llvmContext);
    importer->declareExternalFunction(doubleType, function, context);
    function = OolongFunction("toString", arguments, &llvmContext);
    importer->declareExternalFunction(stringType, function, context);

    // Double functions
    arguments.clear();
    arguments.push_back(doubleType);
    function = OolongFunction("toBoolean", arguments, &llvmContext);
    importer->declareExternalFunction(booleanType, function, context);
    function = OolongFunction("toInteger", arguments, &llvmContext);
    importer->declareExternalFunction(integerType, function, context);
    function = OolongFunction("toString", arguments, &llvmContext);
    importer->declareExternalFunction(stringType, function, context);

    // String functions
    arguments.clear();
    arguments.push_back(stringType);
    function = OolongFunction("toBoolean", arguments, &llvmContext);
    importer->declareExternalFunction(booleanType, function, context);
    function = OolongFunction("toInteger", arguments, &llvmContext);
    importer->declareExternalFunction(integerType, function, context);
    function = OolongFunction("toDouble", arguments, &llvmContext);
    importer->declareExternalFunction(doubleType, function, context);
}

static void createIoFunctions(Importer* importer, CodeGenerationContext* context) {
    LLVMContext& llvmContext = context->getLLVMContext();

    Type* voidType = Type::getVoidTy(llvmContext);
    Type* stringType = TypeConverter::getStringType(llvmContext);

    // print String
    vector<Type*> arguments;
    arguments.push_back(Type::getInt8PtrTy(llvmContext));
    OolongFunction function("io.print", arguments, &llvmContext);
    importer->declareExternalFunction(voidType, function, context);

    // print Integer
    arguments.clear();
    arguments.push_back(Type::getInt64Ty(llvmContext));
    function = OolongFunction("io.print", arguments, &llvmContext);
    importer->declareExternalFunction(voidType, function, context);

    // print Double
    arguments.clear();
    arguments.push_back(Type::getDoubleTy(llvmContext));
    function = OolongFunction("io.print", arguments, &llvmContext);
    importer->declareExternalFunction(voidType, function, context);

    // print Boolean
    arguments.clear();
    arguments.push_back(Type::getInt1Ty(llvmContext));
    function = OolongFunction("io.print", arguments, &llvmContext);
    importer->declareExternalFunction(voidType, function, context);

    // printLine String
    arguments.clear();
    arguments.push_back(Type::getInt8PtrTy(llvmContext));
    function = OolongFunction("io.printLine", arguments, &llvmContext);
    importer->declareExternalFunction(voidType, function, context);

    // printLine Integer
    arguments.clear();
    arguments.push_back(Type::getInt64Ty(llvmContext));
    function = OolongFunction("io.printLine", arguments, &llvmContext);
    importer->declareExternalFunction(voidType, function, context);

    // printLine Double
    arguments.clear();
    arguments.push_back(Type::getDoubleTy(llvmContext));
    function = OolongFunction("io.printLine", arguments, &llvmContext);
    importer->declareExternalFunction(voidType, function, context);

    // printLine Boolean
    arguments.clear();
    arguments.push_back(Type::getInt1Ty(llvmContext));
    function = OolongFunction("io.printLine", arguments, &llvmContext);
    importer->declareExternalFunction(voidType, function, context);

    // readLine
    arguments.clear();
    function = OolongFunction("io.readLine", arguments, &llvmContext);
    importer->declareExternalFunction(stringType, function, context);
}

