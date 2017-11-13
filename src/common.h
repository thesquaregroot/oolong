#ifndef COMMON_H
#define COMMON_H

#include "code-generation.h"
#include "llvm/IR/Value.h"
#include <string>

llvm::Value* warning(CodeGenerationContext& context, const std::string& warningMessage);
llvm::Value* error(CodeGenerationContext& context, const std::string& errorMessage);

#endif
