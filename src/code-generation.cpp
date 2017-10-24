#include "node.h"
#include "code-generation.h"
#include "parser.hpp"
#include <vector>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/Support/raw_ostream.h>

using namespace std;
using namespace llvm;

CodeGenerationContext::CodeGenerationContext() {
    this->llvmContext = new LLVMContext();
    this->module = new Module("main", *(this->llvmContext));
}

/* Compile the AST into a module */
void CodeGenerationContext::generateCode(ProgramNode& root)
{
	std::cout << "Generating code...\n";

	root.generateCode(*this);

	/* Print the bytecode in a human-readable format
	   to see if our program compiled properly
	 */
	std::cout << "Code is generated.\n";
	//module->dump();

	legacy::PassManager pm;
	pm.add(createPrintModulePass(outs()));
	pm.run(*module);
}

/* Executes the AST by running the main function */
GenericValue CodeGenerationContext::runCode() {
	std::cout << "Running code...\n";
	ExecutionEngine *ee = EngineBuilder( unique_ptr<Module>(module) ).create();
	ee->finalizeObject();
	vector<GenericValue> noargs;
	GenericValue v = ee->runFunction(mainFunction, noargs);
	std::cout << "Code was run.\n";
	return v;
}

std::map<std::string, Value*>& CodeGenerationContext::locals() {
    return blocks.top()->locals;
}

BasicBlock *CodeGenerationContext::currentBlock() {
    return blocks.top()->block;
}

LLVMContext& CodeGenerationContext::getLLVMContext() {
    return *llvmContext;
}

Module* CodeGenerationContext::getModule() {
    return module;
}

Function* CodeGenerationContext::getMainFunction() {
    return mainFunction;
}

void CodeGenerationContext::setMainFunction(Function* function) {
    mainFunction = function;
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

void CodeGenerationContext::setCurrentReturnValue(Value *value) {
    blocks.top()->returnValue = value;
}

Value* CodeGenerationContext::getCurrentReturnValue() {
    return blocks.top()->returnValue;
}

