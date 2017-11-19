#ifndef TYPE_CONVERTER_H
#define TYPE_CONVERTER_H

#include "llvm/IR/Value.h"
#include "llvm/IR/LLVMContext.h"
#include <string>

class CodeGenerationContext;

class TypeConverter {
private:
    CodeGenerationContext* context;

public:
    TypeConverter(CodeGenerationContext* context) : context(context) {}

    static llvm::Type* getBooleanType(llvm::LLVMContext& llvmContext);
    static llvm::Type* getIntegerType(llvm::LLVMContext& llvmContext);
    static llvm::Type* getDoubleType(llvm::LLVMContext& llvmContext);
    static llvm::Type* getStringType(llvm::LLVMContext& llvmContext);

    llvm::Type* getBooleanType();
    llvm::Type* getIntegerType();
    llvm::Type* getDoubleType();
    llvm::Type* getStringType();

    llvm::Type*  typeOf(const std::string& name);
    std::string  getTypeName(llvm::Type* type);
    bool         canConvertType(llvm::Type* targetType, llvm::Type* value);
    llvm::Value* convertType(llvm::Value* value, llvm::Type* targetType);
};

#endif
