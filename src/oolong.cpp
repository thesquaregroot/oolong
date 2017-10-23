#include<iostream>
#include "node.h"
#include "code-generation.h"

using namespace std;

extern ProgramNode* programNode;
extern int yyparse();

int main(int argc, char **argv) {
    yyparse();

    CodeGenerationContext context;
    context.generateCode(*programNode);
    context.runCode();

    return 0;
}

