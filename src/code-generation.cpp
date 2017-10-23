#include "node.h"
#include "code-generation.h"
#include "parser.hpp"
#include <vector>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/CallingConv.h>
//#include <llvm/Bitcode/ReaderWriter.h>
//#include <llvm/Analysis/Verifier.h>
//#include <llvm/Assembly/PrintModulePass.h>
//#include <llvm/Support/IRBuilder.h>
//#include <llvm/ModuleProvider.h>
//#include <llvm/Target/TargetSelect.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/Support/raw_ostream.h>

using namespace std;
using namespace llvm;

CodeGenerationContext::CodeGenerationContext() {
    this->context = new LLVMContext();
    this->module = new Module("main", *this->context);
}

/* Compile the AST into a module */
void CodeGenerationContext::generateCode(ProgramNode& root) {
    std::cout << "Generating code...\n";

    /* Create the top level interpreter function to call as entry */
    vector<const Type*> argTypes;
    FunctionType *ftype = FunctionType::get(Type::getVoidTy(*this->context), argTypes, false);
    mainFunction = Function::Create(ftype, GlobalValue::InternalLinkage, "main", module);
    BasicBlock *bblock = BasicBlock::Create(*this->context, "entry", mainFunction, 0);

    /* Push a new variable/block context */
    pushBlock(bblock);
    root.generateCode(*this); /* emit bytecode for the toplevel block */
    ReturnInst::Create(*this->context, bblock);
    popBlock();

    /* Print the bytecode in a human-readable format
       to see if our program compiled properly
     */
    std::cout << "Code is generated.\n";
    PassManager pm;
    pm.add(createPrintModulePass(&outs()));
    pm.run(*module);
}

/* Executes the AST by running the main function */
GenericValue CodeGenerationContext::runCode() {
    std::cout << "Running code...\n";
    ExistingModuleProvider *mp = new ExistingModuleProvider(module);
    ExecutionEngine *ee = ExecutionEngine::create(mp, false);
    vector<GenericValue> noargs;
    GenericValue v = ee->runFunction(mainFunction, noargs);
    std::cout << "Code was run.\n";
    return v;
}

/* Returns an LLVM type based on the identifier */
static const Type *typeOf(const IdentifierNode& type) {
    if (type.name.compare("int") == 0) {
        return Type::getInt64Ty(*this->context);
    }
    else if (type.name.compare("double") == 0) {
        return Type::getDoubleTy(this->context);
    }
    return Type::getVoidTy(this->context);
}

std::map<std::string, Value*>& CodeGenerationContext::locals() {
    return blocks.top()->locals;
}

BasicBlock *CodeGenerationContext::currentBlock() {
    return blocks.top()->block;
}

void CodeGenerationContext::pushBlock(BasicBlock *block) {
    blocks.push(new CodeGenerationBlock());
    blocks.top()->block = block;
}

void CodeGenerationContext::popBlock() {
    CodeGenerationBlock *top = blocks.top();
    blocks.pop();
    delete top;
}

