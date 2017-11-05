#include "abstract-syntax-tree.h"
#include "code-generation.h"
#include "oolong.h"
#include <iostream>

using namespace std;
using namespace llvm;

extern int yydebug;
extern int yyparse();
extern BlockNode* programNode;

int main(int argc, char **argv) {
    // enable debug
    //yydebug = 1;

    // parse input
    int parseValue = yyparse();
    if (parseValue > 0) {
        return parseValue;
    }

	CodeGenerationContext context;
	return context.generateCode(*programNode);
}

