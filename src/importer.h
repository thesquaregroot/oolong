#ifndef IMPORTER_H
#define IMPORTER_H

#include "llvm/IR/Function.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/LLVMContext.h"
#include <string>
#include <map>
#include <vector>

class CodeGenerationContext;

class OolongFunction {
private:
    std::string name;
    std::vector<llvm::Type*> arguments;
    llvm::LLVMContext* llvmContext;

public:
    OolongFunction(const std::string& name, const std::vector<llvm::Type*> arguments, llvm::LLVMContext* llvmContext) : name(name), arguments(arguments), llvmContext(llvmContext) {}

    std::string getName() const;
    std::vector<llvm::Type*> getArguments() const;
    size_t getArgumentCount() const;

    bool matches(const OolongFunction& function, bool castArguments) const;
    bool operator==(const OolongFunction& function) const;
    bool operator!=(const OolongFunction& function) const;
    bool operator<(const OolongFunction& function) const;
};

std::string to_string(const OolongFunction& function);

class Importer {
private:
    std::map<OolongFunction, llvm::Function*> importedFunctions;

public:
    void declareFunction(const OolongFunction& function, llvm::Function* functionReference);
    void declareExternalFunction(llvm::Type* returnType, const OolongFunction& function, const char* externalName, CodeGenerationContext* context);
    bool importPackage(const std::string& package, CodeGenerationContext* context);
    llvm::Function* findFunction(const OolongFunction& function) const;
    llvm::Function* findFunction(const OolongFunction& function, bool exactMatch) const;
};

#endif

