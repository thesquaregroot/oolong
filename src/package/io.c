
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

#include <stdio.h>
#include <stdbool.h>

void ___io___print_String(const char* value) {
    printf("%s", value);
}

void ___io___print_Integer(long long value) {
    printf("%d", value);
}

void ___io___print_Double(double value) {
    printf("%f", value);
}

void ___io___print_Boolean(bool value) {
    printf(value ? "true" : "false");
}

void ___io___printLine_String(const char* value) {
    printf("%s\n", value);
}

void ___io___printLine_Integer(long long value) {
    printf("%d\n", value);
}

void ___io___printLine_Double(double value) {
    printf("%f\n", value);
}

void ___io___printLine_Boolean(bool value) {
    printf(value ? "true\n" : "false\n");
}

char* ___io___readLine() {
    char* line = NULL;
    size_t length = 0;

    ssize_t read = getline(&line, &length, stdin);
    // TODO: handle failed read
    // TODO: ensure line is freed
    return line;
}

