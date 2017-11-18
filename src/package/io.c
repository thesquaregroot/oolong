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

