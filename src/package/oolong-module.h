#ifndef OOLONG_MODULE_H
#define OOLONG_MODULE_H

#include <stdint.h>

struct String {
    char* value;
    int64_t allocatedSize;
    int64_t usedSize;
};

#endif

