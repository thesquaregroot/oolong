
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

#include "oolong-module.h"
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

void Void_0_io_1_print_2_String(struct String* value) {
    printf("%s", value->value);
}

void Void_0_io_1_print_2_Integer(int64_t value) {
    printf("%" PRId64, value);
}

void Void_0_io_1_print_2_Double(double value) {
    printf("%f", value);
}

void Void_0_io_1_print_2_Boolean(bool value) {
    printf(value ? "true" : "false");
}

void Void_0_io_1_printLine_2_String(struct String* value) {
    printf("%s\n", value->value);
}

void Void_0_io_1_printLine_2_Integer(int64_t value) {
    printf("%" PRId64 "\n", value);
}

void Void_0_io_1_printLine_2_Double(double value) {
    printf("%f\n", value);
}

void Void_0_io_1_printLine_2_Boolean(bool value) {
    printf(value ? "true\n" : "false\n");
}

struct String* String_0_io_1_readLine() {
    char* line = NULL;
    size_t length = 0;

    ssize_t read = getline(&line, &length, stdin);
    // TODO: handle failed read
    // TODO: ensure line is freed

    struct String* lineString = malloc(sizeof(struct String));
    // TODO: determine how to handle failed malloc
    lineString->value = line;
    lineString->allocatedSize = read+1; // account for null terminator
    lineString->usedSize = read;

    return lineString;
}

