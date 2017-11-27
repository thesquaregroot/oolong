#ifndef TYPE_CONVERTER_H
#define TYPE_CONVERTER_H

#include "llvm/IR/Value.h"
#include "llvm/IR/LLVMContext.h"
#include <string>
#include <map>

class CodeGenerationContext;

class TypeConverter {
private:
    CodeGenerationContext* context;
    std::map<std::string, llvm::Type*> types;

public:
    TypeConverter(CodeGenerationContext* context) : context(context) {
        types["Void"] = getVoidType();
        types["Boolean"] = getBooleanType();
        types["Integer"] = getIntegerType();
        types["Double"] = getDoubleType();
    }

    static llvm::Type* getVoidType(llvm::LLVMContext& llvmContext);
    static llvm::Type* getBooleanType(llvm::LLVMContext& llvmContext);
    static llvm::Type* getIntegerType(llvm::LLVMContext& llvmContext);
    static llvm::Type* getDoubleType(llvm::LLVMContext& llvmContext);

    llvm::Type* getVoidType();
    llvm::Type* getBooleanType();
    llvm::Type* getIntegerType();
    llvm::Type* getDoubleType();

    llvm::Type*  getType(const std::string& name);
    std::string  getTypeName(llvm::Type* type);
    bool         canConvertType(llvm::Type* targetType, llvm::Type* value);
    llvm::Value* convertType(llvm::Value* value, llvm::Type* targetType);

    llvm::StructType* createType(llvm::ArrayRef<llvm::Type*> members, const std::string& name);
};

#endif
