#ifndef CODE_GENERATION_H
#define CODE_GENERATION_H

#include <stack>
#include <map>

namespace llvm {
    class BasicBlock;
    class LLVMContext;
    class Module;
    class Function;
    class GenericValue;
    class Value;
}
class ProgramNode;

class CodeGenerationBlock {
public:
    llvm::BasicBlock *block;
    std::map<std::string, llvm::Value*> locals;
    llvm::Value *returnValue;
};

class CodeGenerationContext {
private:
    llvm::LLVMContext *llvmContext;
    llvm::Module *module;
    std::stack<CodeGenerationBlock*> blocks;
    llvm::Function *mainFunction;

public:
    CodeGenerationContext();

    void generateCode(ProgramNode& root);
    llvm::GenericValue runCode();
    std::map<std::string, llvm::Value*>& locals();
    llvm::BasicBlock *currentBlock();
    llvm::LLVMContext& getLLVMContext();
    llvm::Module* getModule();
    llvm::Function* getMainFunction();
    void setMainFunction(llvm::Function* function);
    void pushBlock(llvm::BasicBlock *block);
    void popBlock();
    void setCurrentReturnValue(llvm::Value *value);
    llvm::Value* getCurrentReturnValue();
};

#endif

