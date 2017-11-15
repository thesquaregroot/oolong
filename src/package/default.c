#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

static char* auto_lltoa(long long value) {
    int size = (int) ceil(log10(value)); // size in decimal
    size++; // account for null terminator

    char* str = malloc(size);
    if (str == NULL) {
        return NULL;
    }

    snprintf(str, size, "%d", value);
    return str;
}

static char* auto_ftoa(double value) {
    // using snprintf to determine size, could probably be more efficient
    int size = snprintf(NULL, 0, "%f", value);
    size++; // account for null terminator

    char* str = malloc(size);
    if (str == NULL) {
        return NULL;
    }

    snprintf(str, size, "%f", value);
    return str;
}

// Boolean
bool ___toBoolean_Boolean(bool value) {
    return value;
}

bool ___toBoolean_Integer(long long value) {
    return value != 0;
}

bool ___toBoolean_Double(double value) {
    return value != 0.0;
}

bool ___toBoolean_String(const char* value) {
    return (strcmp(value, "true") == 0);
}

// Integer
long long ___toInteger_Integer(long long value) {
    return value;
}

long long ___toInteger_Boolean(bool value) {
    return value ? 1 : 0;
}

long long ___toInteger_Double(double value) {
    return (long long) value;
}

long long ___toInteger_String(const char* value) {
    return atoll(value);
}

// Double
double ___toDouble_Double(double value) {
    return value;
}

double ___toDouble_Boolean(bool value) {
    return value ? 1.0 : 0.0;
}

double ___toDouble_Integer(long long value) {
    return (double) value;
}

double ___toDouble_String(const char* value) {
    return atof(value);
}

// String
const char* ___toString_String(const char* value) {
    return value;
}

const char* ___toString_Boolean(bool value) {
    return value ? "true" : "false";
}

const char* ___toString_Integer(long long value) {
    return auto_lltoa(value);
}

const char* ___toString_Double(double value) {
    return auto_ftoa(value);
}

