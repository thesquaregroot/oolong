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
    CodeGenerationContext* context;

    friend std::string to_string(const OolongFunction& function);

public:
    OolongFunction(const std::string& name, const std::vector<llvm::Type*> arguments, CodeGenerationContext* context) : name(name), arguments(arguments), context(context) {}

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
    CodeGenerationContext* context;
    std::map<OolongFunction, llvm::Function*> importedFunctions;

public:
    Importer(CodeGenerationContext* context) : context(context) {}

    void declareFunction(const OolongFunction& function, llvm::Function* functionReference);
    void declareExternalFunction(llvm::Type* returnType, const OolongFunction& function);
    bool importPackage(const std::string& package);
    llvm::Function* findFunction(const OolongFunction& function) const;
    llvm::Function* findFunction(const OolongFunction& function, bool exactMatch) const;
};

#endif

