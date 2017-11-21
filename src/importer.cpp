
// Oolong - A compiler for the Oolong programming language.
// Copyright (C) 2017  Andrew Groot
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

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
Type* OolongFunction::getReturnType() const {
    return returnType;
}

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
            if (!context->getTypeConverter().canConvertType(targetType, other.arguments[i])) {
                return false;
            }
        }
        else {
            if (targetType != other.arguments[i]) {
                return false;
            }
        }
    }
    // ignore return type
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
    // ignore return type
    // equal
    return false;
}

string to_string(const OolongFunction& function) {
    auto typeConverter = function.context->getTypeConverter();
    stringstream str;
    str << function.getName();
    str << "(";
    auto arguments = function.getArguments();
    for (size_t i=0; i<arguments.size(); i++) {
        if (i != 0) {
            str << ", ";
        }
        str << typeConverter.getTypeName(arguments[i]);
    }
    str << ") : ";
    str << typeConverter.getTypeName(function.returnType);
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

void Importer::declareExternalFunction(const OolongFunction& function) {
    string externalName = function.getName();
    // replace dots
    replaceAll(externalName, ".", EXTERNAL_FUNCTION_PACKAGE_SEPARATOR);
    // add prefix
    externalName = EXTERNAL_FUNCTION_PREFIX + externalName;
    // add arguments to name
    for (Type* argumentType : function.getArguments()) {
        externalName += EXTERNAL_FUNCTION_ARGUMENT_SEPARATOR + context->getTypeConverter().getTypeName(argumentType);
    }

    FunctionType* functionType = FunctionType::get(function.getReturnType(), function.getArguments(), true);
    Function* functionReference = Function::Create(functionType, Function::ExternalLinkage, Twine(externalName), context->getModule());
    functionReference->setCallingConv(CallingConv::C);

    importedFunctions[function] = functionReference;

    //cout << "Declared " << to_string(function) << endl;
}

bool Importer::importPackage(const string& package) {
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
    OolongFunction function(integerType, "toInteger", arguments, context);
    importer->declareExternalFunction(function);
    function = OolongFunction(doubleType, "toDouble", arguments, context);
    importer->declareExternalFunction(function);
    function = OolongFunction(stringType, "toString", arguments, context);
    importer->declareExternalFunction(function);

    // Integer functions
    arguments.clear();
    arguments.push_back(integerType);
    function = OolongFunction(booleanType, "toBoolean", arguments, context);
    importer->declareExternalFunction(function);
    function = OolongFunction(doubleType, "toDouble", arguments, context);
    importer->declareExternalFunction(function);
    function = OolongFunction(stringType, "toString", arguments, context);
    importer->declareExternalFunction(function);

    // Double functions
    arguments.clear();
    arguments.push_back(doubleType);
    function = OolongFunction(booleanType, "toBoolean", arguments, context);
    importer->declareExternalFunction(function);
    function = OolongFunction(integerType, "toInteger", arguments, context);
    importer->declareExternalFunction(function);
    function = OolongFunction(stringType, "toString", arguments, context);
    importer->declareExternalFunction(function);

    // String functions
    arguments.clear();
    arguments.push_back(stringType);
    function = OolongFunction(booleanType, "toBoolean", arguments, context);
    importer->declareExternalFunction(function);
    function = OolongFunction(integerType, "toInteger", arguments, context);
    importer->declareExternalFunction(function);
    function = OolongFunction(doubleType, "toDouble", arguments, context);
    importer->declareExternalFunction(function);
}

static void createIoFunctions(Importer* importer, CodeGenerationContext* context) {
    LLVMContext& llvmContext = context->getLLVMContext();

    Type* voidType = Type::getVoidTy(llvmContext);
    Type* stringType = TypeConverter::getStringType(llvmContext);

    // print String
    vector<Type*> arguments;
    arguments.push_back(Type::getInt8PtrTy(llvmContext));
    OolongFunction function(voidType, "io.print", arguments, context);
    importer->declareExternalFunction(function);

    // print Integer
    arguments.clear();
    arguments.push_back(Type::getInt64Ty(llvmContext));
    function = OolongFunction(voidType, "io.print", arguments, context);
    importer->declareExternalFunction(function);

    // print Double
    arguments.clear();
    arguments.push_back(Type::getDoubleTy(llvmContext));
    function = OolongFunction(voidType, "io.print", arguments, context);
    importer->declareExternalFunction(function);

    // print Boolean
    arguments.clear();
    arguments.push_back(Type::getInt1Ty(llvmContext));
    function = OolongFunction(voidType, "io.print", arguments, context);
    importer->declareExternalFunction(function);

    // printLine String
    arguments.clear();
    arguments.push_back(Type::getInt8PtrTy(llvmContext));
    function = OolongFunction(voidType, "io.printLine", arguments, context);
    importer->declareExternalFunction(function);

    // printLine Integer
    arguments.clear();
    arguments.push_back(Type::getInt64Ty(llvmContext));
    function = OolongFunction(voidType, "io.printLine", arguments, context);
    importer->declareExternalFunction(function);

    // printLine Double
    arguments.clear();
    arguments.push_back(Type::getDoubleTy(llvmContext));
    function = OolongFunction(voidType, "io.printLine", arguments, context);
    importer->declareExternalFunction(function);

    // printLine Boolean
    arguments.clear();
    arguments.push_back(Type::getInt1Ty(llvmContext));
    function = OolongFunction(voidType, "io.printLine", arguments, context);
    importer->declareExternalFunction(function);

    // readLine
    arguments.clear();
    function = OolongFunction(stringType, "io.readLine", arguments, context);
    importer->declareExternalFunction(function);
}

