
// Oolong - A compiler for the Oolong programming language.
// Copyright (C) 2017  Andrew Groot
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "abstract-syntax-tree.h"
#include "code-generation.h"
#include "oolong.h"
#include <llvm/ExecutionEngine/GenericValue.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <cstdio>
#include <cctype>
#include <cstdio>
#include <cstdlib>
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
    cout << "Oolong, version " << OOLONG_MAJOR_VERSION << "." << OOLONG_MINOR_VERSION << '\n'
         << '\n'
         << "Copyright (C) 2017  Andrew Groot\n"
         << "This program comes with ABSOLUTELY NO WARRANTY.\n"
         << "This is free software, and you are welcome to redistribute it\n"
         << "under certain conditions.  Type `oolong --license' for details." << endl;
}

static void printLicense() {
    cout << "Oolong - A compiler for the Oolong programming language.\n"
         << "Copyright (C) 2017  Andrew Groot\n"
         << '\n'
         << "===========================================================================\n"
         << '\n'
         << OOLONG_LICENSE << endl;
}

static void printUsage() {
    printVersion();
    cout << '\n'
         << "   oolong <options>                    Read from stdin.\n"
         << "   oolong <options> <input-files>      Read <input-files>.\n"
         << '\n'
         << "Options\n"
         << "   --version                   Print version information.\n"
         << "   --license                   Print license information.\n"
         << "   -h, --help                  Print this information.\n"
         << "   -d, --debug                 Enable debug output.\n"
         << "   -b, --bison-debug           Enable bison debug output.\n"
         << "   -l, --emit-llvm             Do not link, output LLVM IR.\n"
         << "   -e, --execute               Do not create any artifacts, execute code directly.\n"
         << "   -c, --compile-only          Do not link, output object files.\n"
         << "   -o, --output-file <file>    Set output file name.\n"
         << "   -O[N]                       Optimize output. N:\n"
         << "                                   0 -> No optimization.\n"
         << "                                   1 -> Run few optimizations for a quicker compile time.\n"
         << "                                   2 -> Run most optimizations. (default)\n"
         << "                                   3 -> Run even optimizations that may be very slow.\n"
         << endl;
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
        return path.substr(lastSlash+1);
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
    command << " lib/* -lm";
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
    int optimizationLevel = 2;
    string outputFile = DEFAULT_OUTPUT_FILE;
    vector<string> inputFiles;

    //// collect arguments
    for (int i=1; i<argc; i++) {
        string argument(argv[i]);

        if (match(argument, nullptr, "--version")) {
            printVersion();
            return 0;
        }
        if (match(argument, nullptr, "--license")) {
            printLicense();
            return 0;
        }
        else if (match(argument, "-h", "--help")) {
            printUsage();
            return 0;
        }
        else if (match(argument, "-d", "--debug")) {
            debug = true;
        }
        else if (match(argument, "-b", "--bison-debug")) {
            yydebug = 1;
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
                cerr << "Output file not specified." << endl;
                return 1;
            }
            outputFile = string(argv[i]);
        }
        else {
            if (argument.find("-O") == 0) {
                if (argument.length() != 3 || !isdigit(argument[2])) {
                    cerr << "Invalid optimizer setting." << endl;
                    return 1;
                }
                // optimization setting
                optimizationLevel = (argument[2] - '0'); // convert digit to int
                continue;
            }
            // assume input file name
            inputFiles.push_back(argument);
        }
    }
    //// collect arguments (end)

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
        if (debug) {
            cout << "Creating temporary directory for generated object files..." << endl;
        }
        tempDirPath = string(mkdtemp(tempDirTemplate));
        if (debug) {
            cout << "Created temporary directory " << tempDirPath << endl;
        }
    }
    vector<string> objectFiles;
    int errorCode = 0;
    for (string inputFile : inputFiles) {
        if (isObjectFile(inputFile)) {
            // no need to do anything, just add it to the object files vector
            objectFiles.push_back(inputFile);
            if (debug) {
                cout << "Added object file " << inputFile << endl;
            }
            continue;
        }

        // parse input
        int parseValue = 0;
        string moduleName = "stdin.ool";
        string outputName = "stdin.ool.o";
        if (inputFile == STDIN_INDICATOR) {
            currentParseFile = "<stdin>";

            setYyin(stdin);
            if (debug) {
                cout << "Parsing file " << currentParseFile << endl;
            }
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
            if (debug) {
                cout << "Parsing file " << currentParseFile << endl;
            }
            parseValue = yyparse();
            fclose(fp);

            // use provided name
            moduleName = inputFile;
            outputName = inputFile + ".o";
        }
        if (link) {
            // no need to put output files in-place, create temp files
            outputName = tempDirPath + '/' + getFileName(outputName);
        }
        else {
            // not linking, outputFile should be used instead, if changed
            if (outputFile != DEFAULT_OUTPUT_FILE) {
                outputName = outputFile;
                if (debug) {
                    cout << "Output file set to " << outputName << endl;
                }
            }
        }
        objectFiles.push_back(outputName);

        // don't continue if parse failed
        if (parseValue > 0) {
            errorCode = parseValue;
            if (debug) {
                cout << "Parse failed with value " << parseValue << " continuing to next file..." << endl;
            }
            continue;
        }

        CodeGenerationContext context(moduleName);
        context.setEmitLlvm(emitLlvm);
        context.setOutputName(outputName);
        context.setOptimizationLevel(optimizationLevel);
        int returnValue = context.generateCode(*programNode);
        if (returnValue > 0) {
            // unable to generate code, bail out
            errorCode = returnValue;
            if (debug) {
                cout << "Code generation failed with value " << returnValue << ".  Bailing out." << endl;
            }
            break;
        }
        if (execute) {
            if (debug) {
                cout << "Execution failed with value " << context.runCode().IntVal.toString(10, true) << "." << endl;
            }
            errorCode = context.runCode().IntVal.getLimitedValue(0);
            break;
        }
    }
    if (link) {
        if (debug) {
            cout << "Linking files..." << endl;
        }
        linkFiles(outputFile, objectFiles);
        if (debug) {
            cout << "Removing temporary files..." << endl;
        }
        removeTempDirectory(tempDirPath);
    }
    return errorCode;
}

