
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

#include "common.h"
#include "type-converter.h"
#include <iostream>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/IR/Constants.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/Error.h>

using namespace std;
using namespace llvm;

Type* TypeConverter::getVoidType(LLVMContext& llvmContext) {
    return Type::getVoidTy(llvmContext);
}

Type* TypeConverter::getBooleanType(LLVMContext& llvmContext) {
    return Type::getInt1Ty(llvmContext);
}

Type* TypeConverter::getIntegerType(LLVMContext& llvmContext) {
    return Type::getInt64Ty(llvmContext);
}

Type* TypeConverter::getDoubleType(LLVMContext& llvmContext) {
    return Type::getDoubleTy(llvmContext);
}

Type* TypeConverter::getVoidType() {
    return TypeConverter::getVoidType(context->getLLVMContext());
}

Type* TypeConverter::getBooleanType() {
    return TypeConverter::getBooleanType(context->getLLVMContext());
}

Type* TypeConverter::getIntegerType() {
    return TypeConverter::getIntegerType(context->getLLVMContext());
}

Type* TypeConverter::getDoubleType() {
    return TypeConverter::getDoubleType(context->getLLVMContext());
}

Type* TypeConverter::getType(const string& name) {
    if (types.find(name) == types.end()) {
        error(*context, "Unable to find type with name: " + name);
        return nullptr;
    }
    Type* type = types[name];
    return type;
}

/* Build a string that represents the provided type */
string TypeConverter::getTypeName(Type* type) {
    if (type == nullptr) {
        return "[nullptr]";
    }
    switch (type->getTypeID()) {
        case Type::TypeID::VoidTyID:        return "Void";
        case Type::TypeID::DoubleTyID:      return "Double";
        case Type::TypeID::IntegerTyID:     {
                                                IntegerType* integerType = (IntegerType*) type;
                                                switch (integerType->getBitWidth()) {
                                                    case 1: return "Boolean";
                                                    case 8: return "Character";
                                                    case 64: return "Integer";
                                                    default: return "Integer[" + to_string(integerType->getBitWidth()) + "]";
                                                }
                                            } break;
        case Type::TypeID::ArrayTyID:       {
                                                // TODO: determine external behavior
                                                return "Array<" + getTypeName(type->getArrayElementType()) + ">";
                                            }
        case Type::TypeID::PointerTyID:     {
                                                Type* pointerElementType = type->getPointerElementType();
                                                if (pointerElementType->isStructTy()) {
                                                    // always use pointers for structs, no need to put in name
                                                    return getTypeName(pointerElementType);
                                                }
                                                return "Pointer<" + getTypeName(pointerElementType) + ">";
                                            }
        case Type::TypeID::StructTyID:      return type->getStructName().str();
        default: {
            // not prepared for this, use stream conversion
            string typeString;
            llvm::raw_string_ostream stream(typeString);
            type->print(stream);
            return typeString;
        }
    }
}

bool TypeConverter::canConvertType(Type* sourceType, Type* targetType) {
    //cout << "Checking conversion from " << getTypeName(sourceType) << " to " << getTypeName(targetType) << endl;

    if (sourceType == targetType) {
        return true;
    }

    Type* integerType = getIntegerType();
    Type* doubleType = getDoubleType();

    // Double target
    if (targetType == doubleType) {
        // convert Integer to Double
        if (sourceType == integerType) {
            return true;
        }
    }

    return false;
}

Value* TypeConverter::convertType(Value* value, Type* targetType) {
    //cout << "Attempting conversion from " << getTypeName(sourceType) << " to " << getTypeName(targetType) << endl;

    Type* valueType = value->getType();
    if (targetType == valueType) {
        // same type pass through
        return value;
    }

    Type* integerType = getIntegerType();
    Type* doubleType = getDoubleType();

    // double target
    if (targetType == doubleType) {
        // convert Integer to Double
        if (valueType == integerType) {
            CastInst* cast = new SIToFPInst(value, doubleType, "", context->currentBlock());
            return cast;
        }
    }

    return error(*context, "No valid conversion found for " + getTypeName(valueType) + " to " + getTypeName(targetType));
}

StructType* TypeConverter::createType(ArrayRef<Type*> members, const string& name) {
    StructType* newType = StructType::create(context->getLLVMContext(), members, name, true);
    // use pointer to the struct as the type (i.e. reference types)
    types[name] = newType->getPointerTo();
    return newType;
}

