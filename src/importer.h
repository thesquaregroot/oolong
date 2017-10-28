#ifndef IMPORTER_H
#define IMPORTER_H

#include "code-generation.h"
#include <map>

class Importer {
public:
    bool importPackage(const std::string& package, CodeGenerationContext& context) const;
};

#endif

