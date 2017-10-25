#include<iostream>
#include "node.h"
#include "code-generation.h"
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/Support/TargetSelect.h>

using namespace std;
using namespace llvm;

extern int yydebug;
extern int yyparse();
extern ProgramNode* programNode;

int main(int argc, char **argv) {
    // enable debug
    //yydebug = 1;
    // parse input
    int parseValue = yyparse();
    if (parseValue > 0) {
        return parseValue;
    }

	CodeGenerationContext context;
	context.generateCode(*programNode);
	context.runCode();

    return 0;
}

