
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
#include "common.h"
#include "parser.hpp"
#include "importer.h"
#include <iostream>
#include <sstream>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/IR/Constants.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/Error.h>

using namespace std;
using namespace llvm;

/* Return the fully formed reference name (joined with periods) */
string createReferenceName(const IdentifierList& reference) {
    stringstream referenceStream;
    for (size_t i = 0; i<reference.size(); i++) {
        if (i != 0) {
            referenceStream << ".";
        }
        referenceStream << reference[i]->name;
    }
    return referenceStream.str();
}

Value* BooleanNode::generateCode(CodeGenerationContext& context) {
    if (value) {
        return ConstantInt::getTrue(context.getLLVMContext());
    } else {
        return ConstantInt::getFalse(context.getLLVMContext());
    }
}

Value* IntegerNode::generateCode(CodeGenerationContext& context) {
    return ConstantInt::get(Type::getInt64Ty(context.getLLVMContext()), value, true);
}

Value* DoubleNode::generateCode(CodeGenerationContext& context) {
    return ConstantFP::get(Type::getDoubleTy(context.getLLVMContext()), value);
}

Value* StringNode::generateCode(CodeGenerationContext& context) {
    // create global constant and return a pointer to it
    LLVMContext& llvmContext = context.getLLVMContext();
    auto stringConstant = ConstantDataArray::getString(llvmContext, value, true);
    Type* type = stringConstant->getType();
    Constant *zero = Constant::getNullValue(IntegerType::getInt32Ty(llvmContext));
    vector<llvm::Value*> indices;
    indices.push_back(zero);
    indices.push_back(zero);
    // make global reference to constant
    GlobalVariable *global = new GlobalVariable(*context.getModule(), type, true, GlobalValue::PrivateLinkage, stringConstant, ".str");
    // get a pointer to the global constant
    GetElementPtrInst* pointer = GetElementPtrInst::CreateInBounds(type, global, makeArrayRef(indices), "", context.currentBlock());
    return pointer;
}

Value* IdentifierNode::generateCode(CodeGenerationContext& context) {
    auto scope = context.fullScope();
    if (scope.find(name) == scope.end()) {
        return error(context, "Undeclared variable " + name);
    }
    return new LoadInst(scope[name], "", false, context.currentBlock());
}

Value* ReferenceNode::generateCode(CodeGenerationContext& context) {
    const string name = createReferenceName(reference);
    auto scope = context.fullScope();
    if (scope.find(name) == scope.end()) {
        return error(context, "Undeclared variable " + name);
    }
    return new LoadInst(scope[name], "", false, context.currentBlock());
}

Value* FunctionCallNode::generateCode(CodeGenerationContext& context) {
    const string functionName = createReferenceName(reference);
    vector<Value*> callingArguments;
    vector<Type*> callingTypes;
    ExpressionList::const_iterator callIt = arguments.begin();
    while (callIt != arguments.end()) {
        Value* callingArgument = (*callIt)->generateCode(context);
        callingArguments.push_back(callingArgument);
        callingTypes.push_back(callingArgument->getType());
        callIt++;
    }
    OolongFunction targetFunction(nullptr, functionName, callingTypes, &context);
    Function *function = context.getImporter().findFunction(targetFunction);
    if (function == nullptr) {
        return error(context, "No such function: " + to_string(targetFunction));
    }
    vector<Value*> convertedArguments;
    auto declaredIt = function->arg_begin();
    size_t position = 0;
    while (declaredIt != function->arg_end()) {
        Argument* declaredArgument = declaredIt;
        Value* callingArgument = callingArguments[position];
        Value* convertedArgument = context.getTypeConverter().convertType(callingArgument, declaredArgument->getType());
        if (convertedArgument == nullptr) {
            return error(context, "Invalid argument for function " + functionName + " (position " + to_string(position+1) + ")");
        }
        convertedArguments.push_back(convertedArgument);
        declaredIt++;
        position++;
    }
    CallInst *call = CallInst::Create(function, makeArrayRef(convertedArguments), "", context.currentBlock());
    return call;
}

Value* BinaryOperatorNode::generateCode(CodeGenerationContext& context) {
    Value* left = leftHandSide.generateCode(context);
    Value* right = rightHandSide.generateCode(context);

    Type* leftType = left->getType();
    Type* rightType = right->getType();

    bool isInteger = true;
    LLVMContext& llvmContext = context.getLLVMContext();
    TypeConverter& typeConverter = context.getTypeConverter();
    Type* booleanType = typeConverter.getBooleanType(llvmContext);
    Type* integerType = typeConverter.getIntegerType(llvmContext);
    Type* doubleType = typeConverter.getDoubleType(llvmContext);
    // change isInteger if either type is a float
    if (!(leftType == booleanType || leftType == integerType || leftType == doubleType)) {
        // leftType is not integer or float
        return error(context, "Unable to perform binary operation with type " + typeConverter.getTypeName(leftType));
    }
    if (!(rightType == booleanType || rightType == integerType || rightType == doubleType)) {
        // rightType is not integer or double
        return error(context, "Unable to perform binary operation with type " + typeConverter.getTypeName(rightType));
    }
    if (leftType == doubleType || rightType == doubleType) {
        left = typeConverter.convertType(left, doubleType);
        right = typeConverter.convertType(right, doubleType);
        isInteger = false;
    }

    bool isBinary = true;
    Instruction::BinaryOps binaryInstruction;
    CmpInst::Predicate predicate;
    switch (operation) {
        case TOKEN_AND:                         { binaryInstruction = Instruction::And; } break;
        case TOKEN_OR:                          { binaryInstruction = Instruction::Or; } break;
        case TOKEN_PLUS:                        { binaryInstruction = isInteger ? Instruction::Add : Instruction::FAdd; } break;
        case TOKEN_MINUS:                       { binaryInstruction = isInteger ? Instruction::Sub : Instruction::FSub; } break;
        case TOKEN_MULTIPLY:                    { binaryInstruction = isInteger ? Instruction::Mul : Instruction::FMul; } break;
        case TOKEN_DIVIDE:                      { binaryInstruction = isInteger ? Instruction::SDiv : Instruction::FDiv; } break;
        case TOKEN_PERCENT:                     { binaryInstruction = isInteger ? Instruction::SRem : Instruction::FRem; } break;
        case TOKEN_EQUAL_TO:                    { predicate = isInteger ? CmpInst::Predicate::ICMP_EQ : CmpInst::Predicate::FCMP_OEQ; isBinary = false; } break;
        case TOKEN_NOT_EQUAL_TO:                { predicate = isInteger ? CmpInst::Predicate::ICMP_NE : CmpInst::Predicate::FCMP_ONE; isBinary = false; } break;
        case TOKEN_GREATER_THAN:                { predicate = isInteger ? CmpInst::Predicate::ICMP_SGT : CmpInst::Predicate::FCMP_OGT; isBinary = false; } break;
        case TOKEN_GREATER_THAN_OR_EQUAL_TO:    { predicate = isInteger ? CmpInst::Predicate::ICMP_SGE : CmpInst::Predicate::FCMP_OGE; isBinary = false; } break;
        case TOKEN_LESS_THAN:                   { predicate = isInteger ? CmpInst::Predicate::ICMP_SLT : CmpInst::Predicate::FCMP_OLT; isBinary = false; } break;
        case TOKEN_LESS_THAN_OR_EQUAL_TO:       { predicate = isInteger ? CmpInst::Predicate::ICMP_SLE : CmpInst::Predicate::FCMP_OLE; isBinary = false; } break;

        default: {
            return error(context, "Unimplemented operation: " + operation);
        }
    }
    if (isBinary) {
        return BinaryOperator::Create(binaryInstruction, left, right, "", context.currentBlock());
    } else {
        Instruction::OtherOps otherInstruction = isInteger ? Instruction::OtherOps::ICmp : Instruction::OtherOps::FCmp;
        return CmpInst::Create(otherInstruction, predicate, left, right, "", context.currentBlock());
    }
}

Value* UnaryOperatorNode::generateCode(CodeGenerationContext& context) {
    Value* value = expression.generateCode(context);

    Type* type = value->getType();

    LLVMContext& llvmContext = context.getLLVMContext();
    TypeConverter& typeConverter = context.getTypeConverter();
    Type* booleanType = typeConverter.getBooleanType(llvmContext);
    Type* integerType = typeConverter.getIntegerType(llvmContext);
    Type* doubleType = typeConverter.getDoubleType(llvmContext);
    if (!(type == booleanType || type == integerType || type == doubleType)) {
        // expression is not integer or double
        return error(context, "Unable to perform unary operation with type " + typeConverter.getTypeName(type));
    }
    bool isInteger = (type != doubleType);

    Value* zero = isInteger ? ConstantInt::get(type, 0) : ConstantFP::get(doubleType, 0.0);
    switch (operation) {
        case TOKEN_MINUS: {
            // subtract from zero
            Value* zero = isInteger ? ConstantInt::get(integerType, 0) : ConstantFP::get(doubleType, 0.0);
            Instruction::BinaryOps instruction = isInteger ? Instruction::Sub : Instruction::FSub;
            return BinaryOperator::Create(instruction, zero, value, "", context.currentBlock());
        } break;

        case TOKEN_NOT: {
            // compare value with zero (1 == 0 -> 0, 0 == 0 -> 1)
            Instruction::OtherOps otherInstruction = isInteger ? Instruction::OtherOps::ICmp : Instruction::OtherOps::FCmp;
            CmpInst::Predicate predicate = isInteger ? CmpInst::Predicate::ICMP_EQ : CmpInst::Predicate::FCMP_OEQ;
            return CmpInst::Create(otherInstruction, predicate, zero, value, "", context.currentBlock());
        } break;

        default: {
            return error(context, "Unimplemented operation: " + operation);
        }
    }

    return value;
}

Value* AssignableNode::generateCode(CodeGenerationContext& context) {
    auto scope = context.fullScope();
    if (scope.find(identifier.name) == scope.end()) {
        return error(context, "Undeclared variable " + identifier.name);
    }
    Value* variable = scope[identifier.name];
    return variable;
}

Value* AssignmentNode::generateCode(CodeGenerationContext& context) {
    Value* value = rightHandSide.generateCode(context);
    Value* variable = leftHandSide.generateCode(context);
    new StoreInst(value, variable, false, context.currentBlock());
    // return stored value
    return value;
}

Value* BlockNode::generateCode(CodeGenerationContext& context) {
    StatementList::const_iterator it;
    Value *last = nullptr;
    for (it = statements.begin(); it != statements.end(); it++) {
        if (context.currentBlockReturns()) {
            return error(context, "Unreachable code after return statement.");
        }
        last = (**it).generateCode(context);
    }
    return last;
}

Value* ExpressionStatementNode::generateCode(CodeGenerationContext& context) {
    return expression.generateCode(context);
}

Value* ReturnStatementNode::generateCode(CodeGenerationContext& context) {
    Value* returnValue = expression.generateCode(context);
    context.setCurrentReturnValue(returnValue);
    ReturnInst::Create(context.getLLVMContext(), returnValue, context.currentBlock());
    return nullptr; // no value, so unnecessary PHINodes don't get created
}

Value* VariableDeclarationNode::generateCode(CodeGenerationContext& context) {
    auto scope = context.fullScope();
    if (scope.find(id.name) != scope.end()) {
        return error(context, "Redeclaration of variable " + id.name);
    }
    TypeConverter& typeConverter = context.getTypeConverter();
    Type* identifierType = typeConverter.typeOf(type.name);
    AllocaInst *alloc = new AllocaInst(identifierType, 0 /* generic address space */, id.name, context.currentBlock());
    context.localScope()[id.name] = alloc;
    if (assignmentExpression != nullptr) {
        AssignableNode* assignable = new AssignableNode(id);
        AssignmentNode assignmentNode(*assignable, *assignmentExpression);
        assignmentNode.generateCode(context);
    }
    return alloc;
}

Value* FunctionDeclarationNode::generateCode(CodeGenerationContext& context) {
    LLVMContext& llvmContext = context.getLLVMContext();
    TypeConverter& typeConverter = context.getTypeConverter();
    vector<Type*> argumentTypes;
    for (VariableDeclarationNode* argument : arguments) {
        argumentTypes.push_back(typeConverter.typeOf(argument->type.name));
    }

    Type* returnType = typeConverter.typeOf(type.name);
    OolongFunction oolongFunction(returnType, id.name, argumentTypes, &context);
    if (context.getImporter().findFunction(oolongFunction, true) != nullptr) {
        // exact match
        return error(context, "Redefinition of function " + to_string(oolongFunction));
    }
    if (context.getImporter().findFunction(oolongFunction, false) != nullptr) {
        // close match
        warning(context, "Potential conflicting definition of function " + to_string(oolongFunction));
    }
    FunctionType *ftype = FunctionType::get(returnType, makeArrayRef(argumentTypes), false);
    Function *function = nullptr;
    if (id.name == "main") {
        // main function, externally linked
        function = Function::Create(ftype, GlobalValue::ExternalLinkage, id.name.c_str(), context.getModule());
        context.setMainFunction(function);
    }
    else {
        // normal function, internally linked
        function = Function::Create(ftype, GlobalValue::InternalLinkage, id.name.c_str(), context.getModule());
    }

    context.getImporter().declareFunction(oolongFunction, function);

    BasicBlock *bblock = BasicBlock::Create(llvmContext, "entry", function, 0);

    context.pushBlock(bblock);

    // add arguments to function scope
    Function::arg_iterator argumentValueIterator = function->arg_begin();
    for (VariableDeclarationNode* argument : arguments) {
        string argumentName = argument->id.name;
        argument->generateCode(context);

        // associate declaration with function argument
        Argument* argumentValue = argumentValueIterator++;
        argumentValue->setName(argumentName);

        // store value created during argument code generation
        new StoreInst(argumentValue, context.localScope()[argumentName], false, bblock);
    }

    // add code for statements
    block.generateCode(context);

    context.popBlock();

    return function;
}

Value* ExternalFunctionDeclarationNode::generateCode(CodeGenerationContext& context) {
    TypeConverter& typeConverter = context.getTypeConverter();

    Type* returnType = typeConverter.typeOf(type.name);

    vector<Type*> argumentTypes;
    for (VariableDeclarationNode* argument : arguments) {
        argumentTypes.push_back(typeConverter.typeOf(argument->type.name));
    }

    OolongFunction function(returnType, id.name, argumentTypes, &context);
    context.getImporter().declareExternalFunction(function, externalName.name);

    return nullptr;
}

Value* ImportStatementNode::generateCode(CodeGenerationContext& context) {
    const string packageName = createReferenceName(reference);
    context.getImporter().importPackage(packageName);
    return nullptr;
}

Value* IfStatementNode::generateCode(CodeGenerationContext& context) {
    LLVMContext& llvmContext = context.getLLVMContext();

    if (condition == nullptr) {
        // else, just print block
        return block.generateCode(context);
    } else {
        // if
        Value* expressionValue = condition->generateCode(context);
        Type* booleanType = TypeConverter::getBooleanType(llvmContext);
        Value* conditionValue = context.getTypeConverter().convertType(expressionValue, booleanType);
        if (conditionValue == nullptr) {
            return error(context, "Conditional expression must be of type Boolean.");
        }

        BasicBlock* currentBlock = context.currentBlock();
        Function* currentFunction = currentBlock->getParent();

        const bool hasElse = (elseStatement != nullptr);

        BasicBlock* thenBlock = BasicBlock::Create(llvmContext, "then", currentFunction);
        BasicBlock* elseBlock = nullptr;
        if (hasElse) {
            elseBlock = BasicBlock::Create(llvmContext, "else", currentFunction);
        }
        BasicBlock* mergeBlock = BasicBlock::Create(llvmContext, "endif");

        if (hasElse) {
            // if-else
            BranchInst::Create(thenBlock, elseBlock, conditionValue, currentBlock);
        } else {
            // simple if
            BranchInst::Create(thenBlock, mergeBlock, conditionValue, currentBlock);
        }

        context.pushBlock(thenBlock);
        Value* thenValue = block.generateCode(context);
        // jump to end
        thenBlock = context.currentBlock();
        bool thenBlockReturns = context.currentBlockReturns();
        if (!thenBlockReturns) {
            BranchInst::Create(mergeBlock, thenBlock);
        }
        context.popBlock();

        Value* elseValue = nullptr;
        bool elseBlockReturns = false;
        if (hasElse) {
            context.pushBlock(elseBlock);
            elseValue = elseStatement->generateCode(context);
            // jump to end
            elseBlock = context.currentBlock();
            elseBlockReturns = context.currentBlockReturns();
            if (!elseBlockReturns) {
                BranchInst::Create(mergeBlock, elseBlock);
            }
            context.popBlock();
        }

        PHINode* phiNode = nullptr;
        if (thenBlockReturns && elseBlockReturns) {
            // both paths return, mark this block as returning
            context.setCurrentReturnValue(nullptr);
        }
        else {
            // manually pushing back mergeBlock to keep things in order
            currentFunction->getBasicBlockList().push_back(mergeBlock);
            if (hasElse && thenValue != nullptr && elseValue != nullptr) {
                phiNode = PHINode::Create(booleanType, 0, "iftmp", mergeBlock);
                phiNode->addIncoming(thenValue, thenBlock);
                phiNode->addIncoming(elseValue, elseBlock);
            }

            // make mergeBlock the new current block
            context.replaceCurrentBlock(mergeBlock);
        }
        return phiNode;
    }
}

Value* WhileLoopNode::generateCode(CodeGenerationContext& context) {
    LLVMContext& llvmContext = context.getLLVMContext();
    Function* currentFunction = context.currentBlock()->getParent();

    BasicBlock* startBlock = BasicBlock::Create(llvmContext, "loopStart", currentFunction);
    BasicBlock* bodyBlock = BasicBlock::Create(llvmContext, "loopBody", currentFunction);
    BasicBlock* exitBlock = BasicBlock::Create(llvmContext, "loopExit");

    // jump to loop start
    BranchInst::Create(startBlock, context.currentBlock());

    context.pushBlock(startBlock);
    Value* expressionValue = condition->generateCode(context);
    Type* booleanType = TypeConverter::getBooleanType(llvmContext);
    Value* conditionValue = context.getTypeConverter().convertType(expressionValue, booleanType);
    if (conditionValue == nullptr) {
        return error(context, "Conditional expression must be of type Boolean.");
    }
    // exit if condition is false
    BranchInst::Create(bodyBlock, exitBlock, conditionValue, context.currentBlock());
    context.popBlock();

    context.pushBlock(bodyBlock);
    block.generateCode(context);
    // jump back to beginning
    bool blockReturns = context.currentBlockReturns();
    if (!blockReturns) {
        BranchInst::Create(startBlock, context.currentBlock());
    }
    context.popBlock();

    // manually pushing back exitBlock to keep things in order
    currentFunction->getBasicBlockList().push_back(exitBlock);
    // make exitBlock the new current block
    context.replaceCurrentBlock(exitBlock);

    return nullptr;
}

Value* ForLoopNode::generateCode(CodeGenerationContext& context) {
    LLVMContext& llvmContext = context.getLLVMContext();
    Function* currentFunction = context.currentBlock()->getParent();

    BasicBlock* initBlock = BasicBlock::Create(llvmContext, "loopInit", currentFunction);
    BasicBlock* startBlock = BasicBlock::Create(llvmContext, "loopStart", currentFunction);
    BasicBlock* bodyBlock = BasicBlock::Create(llvmContext, "loopBody", currentFunction);
    BasicBlock* exitBlock = BasicBlock::Create(llvmContext, "loopExit");

    // jump to loop initializer
    BranchInst::Create(initBlock, context.currentBlock());

    context.pushBlock(initBlock);
    initializer->generateCode(context);
    // jump to condition
    BranchInst::Create(startBlock, context.currentBlock());

    context.pushBlock(startBlock);
    Value* expressionValue = condition->generateCode(context);
    Type* booleanType = TypeConverter::getBooleanType(llvmContext);
    Value* conditionValue = context.getTypeConverter().convertType(expressionValue, booleanType);
    if (conditionValue == nullptr) {
        return error(context, "Conditional expression must be of type Boolean.");
    }
    // exit if condition is false
    BranchInst::Create(bodyBlock, exitBlock, conditionValue, context.currentBlock());
    context.popBlock(); // startBlock

    context.pushBlock(bodyBlock);
    block.generateCode(context);
    // jump back to beginning
    bool blockReturns = context.currentBlockReturns();
    if (!blockReturns) {
        afterthought->generateCode(context);
        BranchInst::Create(startBlock, context.currentBlock());
    }
    context.popBlock(); // bodyBlock
    context.popBlock(); // initBlock (descope initializer variable)

    // manually pushing back exitBlock to keep things in order
    currentFunction->getBasicBlockList().push_back(exitBlock);
    // make exitBlock the new current block
    context.replaceCurrentBlock(exitBlock);

    return nullptr;
}

Value* IncrementExpressionNode::generateCode(CodeGenerationContext& context) {
    ReferenceNode* variableReference = new ReferenceNode(assignable);

    Value* originalValue = nullptr;
    if (postfix) {
        // need to return original value
        originalValue = variableReference->generateCode(context);
    }
    IntegerNode* one = new IntegerNode(1);
    BinaryOperatorNode* add = new BinaryOperatorNode(*variableReference, TOKEN_PLUS, *one);
    AssignmentNode store(assignable, *add);

    Value* incrementedValue = store.generateCode(context);
    if (postfix) {
        // return original value
        return originalValue;
    }
    else {
        // prefix, return incremented value
        return incrementedValue;
    }
}

Value* DecrementExpressionNode::generateCode(CodeGenerationContext& context) {
    ReferenceNode* variableReference = new ReferenceNode(assignable);

    Value* originalValue = nullptr;
    if (postfix) {
        // need to return original value
        originalValue = variableReference->generateCode(context);
    }
    IntegerNode* one = new IntegerNode(1);
    BinaryOperatorNode* add = new BinaryOperatorNode(*variableReference, TOKEN_MINUS, *one);
    AssignmentNode store(assignable, *add);

    Value* incrementedValue = store.generateCode(context);
    if (postfix) {
        // return original value
        return originalValue;
    }
    else {
        // prefix, return incremented value
        return incrementedValue;
    }
}

