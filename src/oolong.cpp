#include "abstract-syntax-tree.h"
#include "code-generation.h"
#include "oolong.h"
#include <llvm/ExecutionEngine/GenericValue.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <cstdio>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

using namespace std;
using namespace llvm;

extern int yydebug;

extern FILE* yyin;
extern int yyparse();
extern void yyrestart(FILE* fp);

extern BlockNode* programNode;

const char* currentParseFile = nullptr;

static void printVersion() {
    cout << "Oolong, version " << OOLONG_MAJOR_VERSION << "." << OOLONG_MINOR_VERSION << endl;
}

static void printUsage() {
    printVersion();
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
    cout << "   -c, --compile-only          Do not link, output object files." << endl;
    cout << "   -o, --output-file <file>    Set output file name." << endl;
    cout << endl;
}

static bool match(const string& argument, const char* shortOption, const char* longOption) {
    return (shortOption != nullptr && argument == shortOption) ||
            (longOption != nullptr && argument == longOption);
}

static void setYyin(FILE* fp) {
    yyin = fp;
    // clear flex buffer in case of multiple files
    yyrestart(yyin);
}

static string getFileName(const string& path) {
    size_t lastSlash = path.find_last_of("/\\"); // forward or back slash
    if (lastSlash == string::npos) {
        return path;
    }
    else {
        return path.substr(lastSlash);
    }
}

static bool isObjectFile(const string& path) {
    // return true iff the extension is right
    return path.rfind(".o") == path.length() - 2;
}

static void linkFiles(const string& outputFile, vector<string> objectFiles) {
    stringstream command;
    // base command
    command << "gcc -o " << outputFile;
    // add user object files
    for (string objectFile : objectFiles) {
        command << " " << objectFile;
    }
    // add oolong packages
    command << " lib/*.o";
    system(command.str().c_str());
}

static void removeTempDirectory(const string& path) {
    string command = "rm -r " + path;
    system(command.c_str());
}

int main(int argc, char **argv) {
    static string DEFAULT_OUTPUT_FILE = "a.out";

    bool debug = false;
    bool emitLlvm = false;
    bool execute = false;
    bool link = true;
    string outputFile = DEFAULT_OUTPUT_FILE;
    vector<string> inputFiles;

    //// collect arguments
    for (int i=1; i<argc; i++) {
        string argument(argv[i]);

        if (match(argument, nullptr, "--version")) {
            printVersion();
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
            link = false;
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
    //// collect arguments (end)

    // enable debug
    if (debug) {
        yydebug = 1;
    }

    if (execute && inputFiles.size() > 1) {
        cerr << "Execute option is only valid with a single input file." << endl;
        return 1;
    }
    if (!link && inputFiles.size() > 1) {
        cerr << "When not linking, only one input file is permitted." << endl;
        return 1;
    }

    const char* STDIN_INDICATOR = "--";
    if (inputFiles.size() == 0) {
        // indicate that stdin should be used
        inputFiles.push_back(STDIN_INDICATOR);
    }

    string tempDirPath = "";
    if (link) {
        char tempDirTemplate[] = "/tmp/oolong_XXXXXXXX";
        tempDirPath = string(mkdtemp(tempDirTemplate));
    }
    vector<string> objectFiles;
    for (string inputFile : inputFiles) {
        if (isObjectFile(inputFile)) {
            // no need to do anything, just add it to the object files vector
            objectFiles.push_back(inputFile);
            continue;
        }

        // parse input
        int parseValue = 0;
        string moduleName = "stdin.ool";
        string outputName = "stdin.ool.o";
        if (inputFile == STDIN_INDICATOR) {
            currentParseFile = "<stdin>";

            setYyin(stdin);
            parseValue = yyparse();
        }
        else {
            // open file and use for parse
            currentParseFile = inputFile.c_str();

            FILE* fp = fopen(currentParseFile, "r");
            if (fp == nullptr) {
                currentParseFile = nullptr;
                cerr << "Unable to open file: " << inputFile << endl;
                return 1;
            }

            setYyin(fp);
            parseValue = yyparse();
            fclose(fp);

            // use provided name
            moduleName = inputFile;
            outputName = inputFile + ".o";
        }
        if (link) {
            // no need to put output files in-place, create temp files
            outputName = tempDirPath + getFileName(outputName);
        }
        else {
            // not linking, outputFile should be used instead, if changed
            if (outputFile != DEFAULT_OUTPUT_FILE) {
                outputName = outputFile;
            }
        }
        objectFiles.push_back(outputName);

        // don't continue if parse failed
        if (parseValue > 0) {
            return parseValue;
        }

        CodeGenerationContext context(moduleName);
        context.setEmitLlvm(emitLlvm);
        context.setOutputName(outputName);
        int returnValue = context.generateCode(*programNode);
        if (returnValue > 0) {
            // unable to generate code, bail out
            return returnValue;
        }
        if (execute) {
            return context.runCode().IntVal.getLimitedValue(0);
        }
    }
    if (link) {
        linkFiles(outputFile, objectFiles);
        removeTempDirectory(tempDirPath);
    }
    return 0;
}

