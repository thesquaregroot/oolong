#ifndef CODE_GENERATION_H
#define CODE_GENERATION_H

#include "importer.h"
#include "type-converter.h"
#include <deque>
#include <map>

namespace llvm {
    class BasicBlock;
    class LLVMContext;
    class Module;
    class Function;
    struct GenericValue;
    class Value;
}

class CodeGenerationBlock {
public:
    llvm::BasicBlock *block;
    std::map<std::string, llvm::Value*> locals;
    llvm::Value *returnValue;
    bool hasReturnValue = false;
};

class BlockNode;

class CodeGenerationContext {
private:
    bool emitLlvm = false;
    std::string outputName;
    int optimizationLevel = 2;

    llvm::LLVMContext *llvmContext;
    llvm::Module *module;
    std::deque<CodeGenerationBlock*> blocks; // deque instead of stack to allow or iteration
    llvm::Function *mainFunction;
    TypeConverter typeConverter;
    Importer importer;

    int importStandardLibrary();
    int optimizeModule();
    int emitIntermediateRepresentation();
    int checkModule();
    int emitMachineCode();

public:
    CodeGenerationContext(const std::string& unitName);

    void setEmitLlvm(bool value);
    void setOutputName(const std::string& value);
    void setOptimizationLevel(int optimizationLevel);

    int generateCode(BlockNode& root);
    llvm::GenericValue runCode();
    std::map<std::string, llvm::Value*>& localScope();
    std::map<std::string, llvm::Value*> fullScope();
    llvm::BasicBlock *currentBlock();
    llvm::Function* currentFunction();
    llvm::LLVMContext& getLLVMContext();
    llvm::Module* getModule();
    llvm::Function* getMainFunction();
    void setMainFunction(llvm::Function* function);
    void pushBlock(llvm::BasicBlock *block);
    void popBlock();
    void replaceCurrentBlock(llvm::BasicBlock* block);
    void setCurrentReturnValue(llvm::Value *value);
    llvm::Value* getCurrentReturnValue();
    bool currentBlockReturns();
    TypeConverter& getTypeConverter();
    Importer& getImporter();
};

#endif

