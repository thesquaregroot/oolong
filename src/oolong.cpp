#include "abstract-syntax-tree.h"
#include "code-generation.h"
#include "oolong.h"
#include <llvm/ExecutionEngine/GenericValue.h>
#include <iostream>
#include <vector>
#include <string>
#include <stdio.h>


using namespace std;
using namespace llvm;

extern int yydebug;
extern int yyparse();
extern BlockNode* programNode;

static void printUsage() {
    cout << "Oolong help" << endl;
    cout << endl;
    cout << "   oolong <options>                    Read from stdin." << endl;
    cout << "   oolong <options> <input-files>      Read <input-files>." << endl;
    cout << endl;
    cout << "Options" << endl;
    cout << "   --version                   Print version information." << endl;
    cout << "   -h, --help                  Print this information." << endl;
    cout << "   -d, --debug                 Enable debug output." << endl;
    cout << "   -l, --emit-llvm             Do not link, output LLVM IR." << endl;
    cout << "   -e, --execute               Do not create any artifacts, execute code directly." << endl;
    cout << "   -c, --compile-only          Compile input files but do not run the linker." << endl;
    cout << "   -o, --output-file <file>    Set output file name." << endl;
    cout << endl;
}

static bool match(const string& argument, const char* shortOption, const char* longOption) {
    return (shortOption != nullptr && argument == shortOption) ||
            (longOption != nullptr && argument == longOption);
}

int main(int argc, char **argv) {
    bool debug = false;
    bool emitLlvm = false;
    bool execute = false;
    bool link = true;
    string outputFile = "a.out";
    vector<string> inputFiles;

    for (int i=1; i<argc; i++) {
        string argument(argv[i]);

        if (match(argument, nullptr, "--version")) {
            cout << "Oolong, version " << OOLONG_MAJOR_VERSION << "." << OOLONG_MINOR_VERSION << endl;
            return 0;
        }
        else if (match(argument, "-h", "--help")) {
            printUsage();
            return 0;
        }
        else if (match(argument, "-d", "--debug")) {
            debug = true;
        }
        else if (match(argument, "-l", "--emit-llvm")) {
            emitLlvm = true;
        }
        else if (match(argument, "-e", "--execute")) {
            execute = true;
        }
        else if (match(argument, "-c", "--compile-only")) {
            link = false;
        }
        else if (match(argument, "-o", "--output-file")) {
            // next argument is output file name
            if (++i >= argc) {
                cerr << "Output file not specified.";
                return 1;
            }
            outputFile = string(argv[i]);
        }
        else {
            // assume input file name
            inputFiles.push_back(argument);
        }
    }

    // enable debug
    if (debug) {
        yydebug = 1;
    }

    // parse input
    int parseValue = yyparse();
    if (parseValue > 0) {
        return parseValue;
    }

	CodeGenerationContext context("main.ool");
    context.setEmitLlvm(emitLlvm);
	int returnValue = context.generateCode(*programNode);
    if (returnValue > 0) {
        return returnValue;
    }
    if (execute) {
        context.runCode();
    }
    return 0;
}

