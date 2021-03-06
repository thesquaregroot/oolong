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
    llvm::Type* returnType;
    std::string name;
    std::vector<llvm::Type*> arguments;
    CodeGenerationContext* context;

    friend std::string to_string(const OolongFunction& function);

public:
    OolongFunction(llvm::Type* returnType, const std::string& name, const std::vector<llvm::Type*> arguments, CodeGenerationContext* context) : returnType(returnType), name(name), arguments(arguments), context(context) {}

    llvm::Type* getReturnType() const;
    std::string getName() const;
    std::string getFunctionName() const;
    std::string getPackageName() const;
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
    std::map<std::string, std::vector<OolongFunction>> packages;
    std::map<OolongFunction, llvm::Function*> importedFunctions;

public:
    Importer(CodeGenerationContext* context) : context(context) {}

    void declareFunction(const OolongFunction& function, llvm::Function* functionReference);
    void declareExternalFunction(const OolongFunction& function);
    void declareExternalFunction(const OolongFunction& function, const std::string& externalName);
    void loadStandardLibrary(const std::string& archiveLocation);
    bool importPackage(const std::string& package);
    llvm::Function* findFunction(const OolongFunction& function) const;
    llvm::Function* findFunction(const OolongFunction& function, bool exactMatch) const;
};

#endif

