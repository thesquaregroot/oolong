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
};

class CodeGenerationContext {
private:
    llvm::LLVMContext *context;
    llvm::Module *module;
    std::stack<CodeGenerationBlock*> blocks;
    llvm::Function *mainFunction;

public:
    CodeGenerationContext();

    void generateCode(ProgramNode& root);
    llvm::GenericValue runCode();
    std::map<std::string, llvm::Value*>& locals();
    llvm::BasicBlock *currentBlock();
    void pushBlock(llvm::BasicBlock *block);
    void popBlock();
};

