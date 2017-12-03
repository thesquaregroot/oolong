
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
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

static struct String* auto_lltoa(const long long value) {
    int size = (int) ceil(log10(value)); // size in decimal
    size++; // account for null terminator

    struct String* string = malloc(sizeof(struct String));
    if (string == NULL) {
        // TODO: print error?
        return NULL;
    }

    char* str = malloc(size);
    if (str == NULL) {
        // TODO: print error?
        return NULL;
    }

    snprintf(str, size, "%lld", value);

    string->value = str;
    string->allocatedSize = size;
    string->usedSize = size-1;
    return string;
}

static struct String* auto_ftoa(const double value) {
    // using snprintf to determine size, could probably be more efficient
    int size = snprintf(NULL, 0, "%f", value);
    size++; // account for null terminator

    struct String* string = malloc(sizeof(struct String));
    if (string == NULL) {
        // TODO: print error?
        return NULL;
    }

    char* str = malloc(size);
    if (str == NULL) {
        // TODO: print error?
        return NULL;
    }

    snprintf(str, size, "%f", value);

    string->value = str;
    string->allocatedSize = size;
    string->usedSize = size-1;
    return string;
}

// Boolean
bool Boolean_0_toBoolean_2_Boolean(bool value) {
    return value;
}

bool Boolean_0_toBoolean_2_Integer(long long value) {
    return value != 0;
}

bool Boolean_0_toBoolean_2_Double(double value) {
    return value != 0.0;
}

bool Boolean_0_toBoolean_2_String(struct String* value) {
    return (strcmp(value->value, "true") == 0);
}

// Integer
long long Integer_0_toInteger_2_Integer(long long value) {
    return value;
}

long long Integer_0_toInteger_2_Boolean(bool value) {
    return value ? 1 : 0;
}

long long Integer_0_toInteger_2_Double(double value) {
    return (long long) value;
}

long long Integer_0_toInteger_2_String(struct String* value) {
    return atoll(value->value);
}

// Double
double Double_0_toDouble_2_Double(double value) {
    return value;
}

double Double_0_toDouble_2_Boolean(bool value) {
    return value ? 1.0 : 0.0;
}

double Double_0_toDouble_2_Integer(long long value) {
    return (double) value;
}

double Double_0_toDouble_2_String(struct String* value) {
    return atof(value->value);
}

// String
struct String* String_0_toString_2_String(struct String* value) {
    return value;
}

struct String* String_0_toString_2_Boolean(bool value) {
    struct String* string = malloc(sizeof(struct String));
    if (string == NULL) {
        // TODO: print error?
        return NULL;
    }
    if (value) {
        string->value = "true";
        string->allocatedSize = 5;
        string->usedSize = 4;
    } else {
        string->value = "false";
        string->allocatedSize = 6;
        string->usedSize = 5;
    }
    return string;
}

struct String* String_0_toString_2_Integer(long long value) {
    return auto_lltoa(value);
}

struct String* String_0_toString_2_Double(double value) {
    return auto_ftoa(value);
}

