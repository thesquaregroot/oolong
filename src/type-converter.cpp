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


Type* TypeConverter::getBooleanType(LLVMContext& llvmContext) {
    return Type::getInt1Ty(llvmContext);
}

Type* TypeConverter::getIntegerType(LLVMContext& llvmContext) {
    return Type::getInt64Ty(llvmContext);
}

Type* TypeConverter::getDoubleType(LLVMContext& llvmContext) {
    return Type::getDoubleTy(llvmContext);
}

/* Returns an LLVM type based on the identifier */
Type* TypeConverter::typeOf(const string& name, LLVMContext& llvmContext) {
    if (name == "Boolean") {
        return getBooleanType(llvmContext);
    }
    else if (name == "Integer") {
        return getIntegerType(llvmContext);
    }
    else if (name == "Double") {
        return getDoubleType(llvmContext);
    }
    else if (name == "Void") {
        return Type::getVoidTy(llvmContext);
    }
    return nullptr;
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
        case Type::TypeID::ArrayTyID:       return "Array<" + getTypeName(type->getArrayElementType()) + ">";
        case Type::TypeID::PointerTyID:     return "Pointer<" + getTypeName(type->getPointerElementType()) + ">";
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

bool TypeConverter::canConvertType(Type* sourceType, Type* targetType, LLVMContext* llvmContext) {
    //cout << "Checking conversion from " << getTypeName(sourceType) << " to " << getTypeName(targetType) << endl;

    if (sourceType == targetType) {
        return true;
    }

    Type* integerType = getIntegerType(*llvmContext);
    Type* doubleType = getDoubleType(*llvmContext);


    // double target
    if (targetType == doubleType) {
        // convert Integer to Double
        if (sourceType == integerType) {
            return true;
        }
    }

    // char* target
    if (targetType->getTypeID() == Type::TypeID::PointerTyID && targetType->getPointerElementType()->getTypeID() == Type::TypeID::IntegerTyID) {
        // convert String to char*
        if (sourceType->getTypeID() == Type::TypeID::ArrayTyID && sourceType->getArrayElementType()->getTypeID() == Type::TypeID::IntegerTyID) {
            return true;
        }
    }

    return false;
}

Value* TypeConverter::convertType(Value* value, Type* targetType, CodeGenerationContext* context) {
    //cout << "Attempting conversion from " << getTypeName(sourceType) << " to " << getTypeName(targetType) << endl;

    Type* valueType = value->getType();
    if (targetType == valueType) {
        // same type pass through
        return value;
    }

    LLVMContext& llvmContext = context->getLLVMContext();

    Type* integerType = getIntegerType(llvmContext);
    Type* doubleType = getDoubleType(llvmContext);

    // double target
    if (targetType == doubleType) {
        // convert Integer to Double
        if (valueType == integerType) {
            CastInst* cast = new SIToFPInst(value, doubleType, "", context->currentBlock());
            return cast;
        }
    }

    // char* target
    if (targetType->getTypeID() == Type::TypeID::PointerTyID && targetType->getPointerElementType()->getTypeID() == Type::TypeID::IntegerTyID) {
        // convert String to char*
        if (valueType->getTypeID() == Type::TypeID::ArrayTyID && valueType->getArrayElementType()->getTypeID() == Type::TypeID::IntegerTyID) {
            ConstantDataSequential* valueConstant = (ConstantDataSequential*) value;

            Constant *zero = Constant::getNullValue(IntegerType::getInt32Ty(llvmContext));
            vector<llvm::Value*> indices;
            indices.push_back(zero);
            indices.push_back(zero);
            // make global reference to constant
            GlobalVariable *global = new GlobalVariable(*context->getModule(), valueType, true, GlobalValue::PrivateLinkage, valueConstant, ".str");
            // get a pointer to the global constant
            GetElementPtrInst* pointer = GetElementPtrInst::CreateInBounds(valueType, global, makeArrayRef(indices), "", context->currentBlock());
            return pointer;
        }
    }

    return error(*context, "No valid conversion found for " + getTypeName(valueType) + " to " + getTypeName(targetType));
}

