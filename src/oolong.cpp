#include<iostream>
#include "node.h"
#include "code-generation.h"
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/Support/TargetSelect.h>

using namespace std;
using namespace llvm;

extern int yyparse();
extern ProgramNode* programNode;

int main(int argc, char **argv) {
    yyparse();

    InitializeNativeTarget();
	InitializeNativeTargetAsmPrinter();
	InitializeNativeTargetAsmParser();

	CodeGenerationContext context;
	context.generateCode(*programNode);
	context.runCode();

    return 0;
}

