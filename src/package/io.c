
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
#include <stdio.h>
#include <stdbool.h>

void Void_0_io_1_print_2_String(const char* value) {
    printf("%s", value);
}

void Void_0_io_1_print_2_Integer(long long value) {
    printf("%d", value);
}

void Void_0_io_1_print_2_Double(double value) {
    printf("%f", value);
}

void Void_0_io_1_print_2_Boolean(bool value) {
    printf(value ? "true" : "false");
}

void Void_0_io_1_printLine_2_String(struct String value) {
    printf("%s\n", value.value);
}

void Void_0_io_1_printLine_2_Integer(long long value) {
    printf("%d\n", value);
}

void Void_0_io_1_printLine_2_Double(double value) {
    printf("%f\n", value);
}

void Void_0_io_1_printLine_2_Boolean(bool value) {
    printf(value ? "true\n" : "false\n");
}

struct String String_0_io_1_readLine() {
    char* line = NULL;
    size_t length = 0;

    ssize_t read = getline(&line, &length, stdin);
    // TODO: handle failed read
    // TODO: ensure line is freed

    struct String lineString;
    lineString.value = line;
    lineString.allocatedSize = length;
    lineString.usedSize = length-1; // account for null terminator

    return lineString;
}

