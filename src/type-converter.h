#ifndef TYPE_CONVERTER_H
#define TYPE_CONVERTER_H

#include "llvm/IR/Value.h"
#include "llvm/IR/LLVMContext.h"
#include <string>

class CodeGenerationContext;

class TypeConverter {
private:

public:
    TypeConverter() {}

    static llvm::Type* getBooleanType(llvm::LLVMContext& llvmContext);
    static llvm::Type* getIntegerType(llvm::LLVMContext& llvmContext);
    static llvm::Type* getDoubleType(llvm::LLVMContext& llvmContext);
    static llvm::Type* typeOf(const std::string& name, llvm::LLVMContext& llvmContext);
    static std::string getTypeName(llvm::Type* type);
    static bool canConvertType(llvm::Type* targetType, llvm::Type* value, llvm::LLVMContext* context);
    llvm::Value* convertType(llvm::Value* value, llvm::Type* targetType, CodeGenerationContext* context);
};

#endif
