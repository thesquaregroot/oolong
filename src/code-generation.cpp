#include "abstract-syntax-tree.h"
#include "code-generation.h"
#include "parser.hpp"
#include <vector>
#include <fstream>
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
#include <llvm/IR/Verifier.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Support/FileSystem.h>
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"

using namespace std;
using namespace llvm;

CodeGenerationContext::CodeGenerationContext(const string& unitName) {
    llvmContext = new LLVMContext();
    module = new Module(unitName, *llvmContext);
}

void CodeGenerationContext::setEmitLlvm(bool value) {
    emitLlvm = value;
}

void CodeGenerationContext::setOutputName(const string& value) {
    outputName = value;
}

// Compile the AST into a module
int CodeGenerationContext::generateCode(BlockNode& root) {
	root.generateCode(*this);

    // verify module
    verifyModule(*module);

    // save IR
    std::string intermediateRepresentation;
    raw_string_ostream output(intermediateRepresentation);
    module->print(output, nullptr);

    if (intermediateRepresentation.length() == 0) {
        return 1;
    } else {
        if (emitLlvm) {
            const string llFileName = (module->getName() + ".ll").str();
            ofstream outputFile(llFileName);
            outputFile << intermediateRepresentation;
            outputFile.flush();
            outputFile.close();
        }
    }

    // Initialize the target registry etc.
    InitializeAllTargetInfos();
    InitializeAllTargets();
    InitializeAllTargetMCs();
    InitializeAllAsmParsers();
    InitializeAllAsmPrinters();

    auto targetTriple = sys::getDefaultTargetTriple();
    module->setTargetTriple(targetTriple);

    string error;
    auto target = TargetRegistry::lookupTarget(targetTriple, error);

    // Print an error and exit if we couldn't find the requested target.
    // This generally occurs if we've forgotten to initialize the
    // TargetRegistry or we have a bogus target triple.
    if (!target) {
        errs() << error;
        return 1;
    }

    auto cpu = "generic";
    auto features = "";

    TargetOptions opt;
    auto RM = Optional<Reloc::Model>(Reloc::Model::PIC_);
    auto targetMachine = target->createTargetMachine(targetTriple, cpu, features, opt, RM);

    module->setDataLayout(targetMachine->createDataLayout());

    std::error_code errorCode;
    raw_fd_ostream dest(outputName, errorCode, sys::fs::F_None);

    legacy::PassManager pass;
    auto fileType = TargetMachine::CGFT_ObjectFile;

    if (targetMachine->addPassesToEmitFile(pass, dest, fileType)) {
        errs() << "Target machine can't emit a file of this type";
        return 1;
    }

    pass.run(*module);
    dest.flush();

    return 0;
}

// Executes the AST by running the main function
GenericValue CodeGenerationContext::runCode() {
	std::cout << "### EXECUTING CODE ###\n";
	ExecutionEngine *ee = EngineBuilder( unique_ptr<Module>(module) ).create();
	ee->finalizeObject();
	vector<GenericValue> noargs;
	GenericValue v = ee->runFunction(mainFunction, noargs);
	std::cout << "### CODE EXECUTED ###\n";
	return v;
}

map<string, Value*>& CodeGenerationContext::localScope() {
    return blocks.back()->locals;
}

map<string, Value*> CodeGenerationContext::fullScope() {
    map<string, Value*> scope;

    // const iterate over block in reverse over, adding locals to scope
    auto blockIterator = blocks.crbegin();
    while (blockIterator != blocks.crend()) {
        auto block = *blockIterator;
        scope.insert(block->locals.begin(), block->locals.end());
        blockIterator++;
    }

    return std::move(scope);
}

BasicBlock* CodeGenerationContext::currentBlock() {
    return blocks.back()->block;
}

Function* CodeGenerationContext::currentFunction() {
    return blocks.back()->block->getParent();
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
    auto newBlock = new CodeGenerationBlock();
    newBlock->block = block;
    blocks.push_back(newBlock);
}

void CodeGenerationContext::popBlock() {
    CodeGenerationBlock* back = blocks.back();
    blocks.pop_back();
    delete back;
}

void CodeGenerationContext::replaceCurrentBlock(BasicBlock* block) {
    CodeGenerationBlock* back = blocks.back();
    // replace BasicBlock but leave locals, etc.
    back->block = block;
}

void CodeGenerationContext::setCurrentReturnValue(Value *value) {
    blocks.back()->returnValue = value;
    blocks.back()->hasReturnValue = true;
}

Value* CodeGenerationContext::getCurrentReturnValue() {
    if (blocks.size() == 0) {
        return nullptr;
    }
    return blocks.back()->returnValue;
}

bool CodeGenerationContext::currentBlockReturns() {
    if (blocks.size() == 0) {
        return false;
    }
    return blocks.back()->hasReturnValue;
}

