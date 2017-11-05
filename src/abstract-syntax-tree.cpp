#include "abstract-syntax-tree.h"
#include "code-generation.h"
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

extern const char* currentParseFile;

static const string FUNCTION_PACKAGE_SEPARATOR = "___";

static Value* warning(CodeGenerationContext& context, const string& warningMessage) {
    cerr << "warning: In file " << currentParseFile << ", function " << context.currentFunction()->getName().str() << ": " << warningMessage << endl;
    return nullptr;
}

static Value* error(CodeGenerationContext& context, const string& errorMessage) {
    const string fullError =  "In file " + string(currentParseFile) +  ", function " + context.currentFunction()->getName().str() + ": " + errorMessage;
    context.getLLVMContext().emitError(fullError);
    return nullptr;
}

static Type* getBooleanType(LLVMContext& llvmContext) {
    return IntegerType::get(llvmContext, 1);
}

/* Return the fully formed reference name (joined with periods) */
static string createReferenceName(const IdentifierList& reference) {
    stringstream referenceStream;
    for (size_t i = 0; i<reference.size(); i++) {
        if (i != 0) {
            referenceStream << FUNCTION_PACKAGE_SEPARATOR;
        }
        referenceStream << reference[i]->name;
    }
    return referenceStream.str();
}

/* Returns an LLVM type based on the identifier */
static Type *typeOf(const IdentifierNode& type, LLVMContext& llvmContext)
{
    if (type.name == "Boolean") {
        return Type::getInt1Ty(llvmContext);
    }
    else if (type.name == "Integer") {
        return Type::getInt64Ty(llvmContext);
    }
    else if (type.name == "Double") {
        return Type::getDoubleTy(llvmContext);
    }
    return Type::getVoidTy(llvmContext);
}

/* Build a string that represents the provided type */
static string getTypeName(Type* type) {
    switch (type->getTypeID()) {
        case Type::TypeID::VoidTyID:        return "Void";
        case Type::TypeID::DoubleTyID:      return "Double";
        case Type::TypeID::IntegerTyID:     {
                                                IntegerType* integerType = (IntegerType*) type;
                                                switch (integerType->getBitWidth()) {
                                                    case 1: return "Boolean";
                                                    case 64: return "Integer";
                                                    default: return "Integer[" + to_string(integerType->getBitWidth()) + "]";
                                                }
                                            } break;
        case Type::TypeID::ArrayTyID:       return "Array<" + getTypeName(type->getArrayElementType()) + ">";
        case Type::TypeID::PointerTyID:     return "Pointer<" + getTypeName(type->getPointerElementType()) + ">";
        case Type::TypeID::StructTyID:      return type->getStructName().str();
        default: {
            // not prepared for this, use stream conversion
            string typeString;
            llvm::raw_string_ostream stream(typeString);
            type->print(stream);
            return typeString;
        }
    }
}

static Value* convertType(Type* targetType, Value* value, CodeGenerationContext& context) {
    Type* valueType = value->getType();
    if (targetType == valueType) {
        // same type pass through
        return value;
    }

    LLVMContext& llvmContext = context.getLLVMContext();
    // char* target
    if (targetType->getTypeID() == Type::TypeID::PointerTyID && targetType->getPointerElementType()->getTypeID() == Type::TypeID::IntegerTyID) {
        // convert String to char*
        if (valueType->getTypeID() == Type::TypeID::ArrayTyID && valueType->getArrayElementType()->getTypeID() == Type::TypeID::IntegerTyID) {
            ConstantDataSequential* valueConstant = (ConstantDataSequential*) value;

            Constant *zero = Constant::getNullValue(IntegerType::getInt32Ty(llvmContext));
            vector<llvm::Value*> indices;
            indices.push_back(zero);
            indices.push_back(zero);
            // make global reference to constant
            GlobalVariable *global = new GlobalVariable(*context.getModule(), valueType, true, GlobalValue::PrivateLinkage, valueConstant, ".str");
            // get a pointer to the global constant
            GetElementPtrInst* pointer = GetElementPtrInst::CreateInBounds(valueType, global, makeArrayRef(indices), "", context.currentBlock());
            return pointer;
        }
    }

    return error(context, "No valid conversion found for " + getTypeName(valueType) + " to " + getTypeName(targetType));
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
    Constant* constant = ConstantDataArray::getString(context.getLLVMContext(), value, true);
    return constant;
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
    Function *function = context.getModule()->getFunction(functionName);
    if (function == nullptr) {
        return error(context, "No such function: " + functionName);
    }
    vector<Value*> callingArguments;
    ExpressionList::const_iterator callIt = arguments.begin();
    auto declaredIt = function->arg_begin();
    size_t position = 1;
    while (callIt != arguments.end()) {
        Argument* declaredArgument = declaredIt;
        Value* callingArgument = (*callIt)->generateCode(context);
        Value* convertedArgument = convertType(declaredArgument->getType(), callingArgument, context);
        if (convertedArgument == nullptr) {
            return error(context, "Invalid argument for function " + functionName + " (position " + to_string(position) + ")");
        }
        callingArguments.push_back(convertedArgument);
        callIt++;
        declaredIt++;
        position++;
    }
    CallInst *call = CallInst::Create(function, makeArrayRef(callingArguments), "", context.currentBlock());
    return call;
}

Value* BinaryOperatorNode::generateCode(CodeGenerationContext& context) {
    Instruction::BinaryOps instruction;
    switch (operation) {
        case TOKEN_PLUS:        { instruction = Instruction::Add; } break;
        case TOKEN_MINUS:       { instruction = Instruction::Sub; } break;
        case TOKEN_MULTIPLY:    { instruction = Instruction::Mul; } break;
        case TOKEN_DIVIDE:      { instruction = Instruction::SDiv; } break;

        /* TODO: comparisons */

        default: {
            return error(context, "Unimplemented operation: " + operation);
        }
    }

    return BinaryOperator::Create(instruction, leftHandSide.generateCode(context), rightHandSide.generateCode(context), "", context.currentBlock());
}

Value* AssignmentNode::generateCode(CodeGenerationContext& context) {
    auto scope = context.fullScope();
    if (scope.find(leftHandSide.name) == scope.end()) {
        return error(context, "Undeclared variable " + leftHandSide.name);
    }
    Value* variable = scope[leftHandSide.name];
    Value* value = rightHandSide.generateCode(context);
    return new StoreInst(value, variable, false, context.currentBlock());
}

Value* BlockNode::generateCode(CodeGenerationContext& context) {
    StatementList::const_iterator it;
    Value *last = nullptr;
    for (it = statements.begin(); it != statements.end(); it++) {
        last = (**it).generateCode(context);
    }
    return last;
}

Value* ExpressionStatementNode::generateCode(CodeGenerationContext& context) {
    return expression.generateCode(context);
}

Value* ReturnStatementNode::generateCode(CodeGenerationContext& context) {
    Value *returnValue = expression.generateCode(context);
    ReturnInst::Create(context.getLLVMContext(), returnValue, context.currentBlock());
    context.setCurrentReturnValue(returnValue);
    return returnValue;
}

Value* VariableDeclarationNode::generateCode(CodeGenerationContext& context) {
    auto scope = context.fullScope();
    if (scope.find(id.name) != scope.end()) {
        return error(context, "Redeclaration of variable " + id.name);
    }
    LLVMContext& llvmContext = context.getLLVMContext();
    Type* identifierType = typeOf(type, llvmContext);
    AllocaInst *alloc = new AllocaInst(identifierType, identifierType->getPointerAddressSpace(), id.name, context.currentBlock());
    context.localScope()[id.name] = alloc;
    if (assignmentExpression != nullptr) {
        AssignmentNode assignmentNode(id, *assignmentExpression);
        assignmentNode.generateCode(context);
    }
    return alloc;
}

Value* FunctionDeclarationNode::generateCode(CodeGenerationContext& context) {
    LLVMContext& llvmContext = context.getLLVMContext();
    vector<Type*> argumentTypes;
    VariableList::const_iterator it;
    for (it = arguments.begin(); it != arguments.end(); it++) {
        argumentTypes.push_back(typeOf((**it).type, llvmContext));
    }
    FunctionType *ftype = FunctionType::get(typeOf(type, llvmContext), makeArrayRef(argumentTypes), false);
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
    BasicBlock *bblock = BasicBlock::Create(llvmContext, "entry", function, 0);

    context.pushBlock(bblock);

    // add arguments to function scope
    Function::arg_iterator argumentValues = function->arg_begin();
    for (it = arguments.begin(); it != arguments.end(); it++) {
        (**it).generateCode(context);

        Value* argumentValue = &*argumentValues++;
        argumentValue->setName((*it)->id.name.c_str());
        // don't need reference
        new StoreInst(argumentValue, context.fullScope()[(*it)->id.name], false, bblock);
    }

    // add code for statements
    block.generateCode(context);

    // add return statement to end of current block (may have changed)
    if (context.getCurrentReturnValue() == nullptr) {
        warning(context, "Adding implicit return instruction.");
        ReturnInst::Create(llvmContext, nullptr, context.currentBlock());
    }
    context.popBlock();

    return function;
}

Value* ImportStatementNode::generateCode(CodeGenerationContext& context) {
    const string packageName = createReferenceName(reference);
    Importer importer;
    importer.importPackage(packageName, context);
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
        Type* booleanType = getBooleanType(llvmContext);
        Value* conditionValue = convertType(booleanType, expressionValue, context);
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
        BranchInst::Create(mergeBlock, thenBlock);
        context.popBlock();

        Value* elseValue = nullptr;
        if (hasElse) {
            context.pushBlock(elseBlock);
            elseValue = elseStatement->generateCode(context);
            // jump to end
            elseBlock = context.currentBlock();
            BranchInst::Create(mergeBlock, elseBlock);
            context.popBlock();
        }

        // manually pushing back mergeBlock to keep things in order
        currentFunction->getBasicBlockList().push_back(mergeBlock);
        PHINode* phiNode = nullptr;
        if (hasElse) {
            phiNode = PHINode::Create(booleanType, 0, "iftmp", mergeBlock);
            phiNode->addIncoming(thenValue, thenBlock);
            phiNode->addIncoming(elseValue, elseBlock);
        }

        // make mergeBlock the new current block
        context.replaceCurrentBlock(mergeBlock);

        return phiNode;
    }
}

