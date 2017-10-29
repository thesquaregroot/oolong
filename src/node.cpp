#include "node.h"
#include "code-generation.h"
#include "parser.hpp"
#include "importer.h"
#include <sstream>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/raw_ostream.h>

using namespace std;
using namespace llvm;

static const string FUNCTION_PACKAGE_SEPARATOR = "___";

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
	if (type.name == "Integer") {
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
        case Type::TypeID::IntegerTyID:     return "Integer";
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

static Value* convertType(Argument* declaredArgument, Value* callingArgument, CodeGenerationContext& context) {
    Type* declaredType = declaredArgument->getType();
    Type* callingType = callingArgument->getType();
    if (declaredType == callingType) {
        // same type pass through
        return callingArgument;
    }

    LLVMContext& llvmContext = context.getLLVMContext();
    // char* declared
    if (declaredType->getTypeID() == Type::TypeID::PointerTyID && declaredType->getPointerElementType()->getTypeID() == Type::TypeID::IntegerTyID) {
        // convert String to char*
        if (callingType->getTypeID() == Type::TypeID::ArrayTyID && callingType->getArrayElementType()->getTypeID() == Type::TypeID::IntegerTyID) {
            ConstantDataSequential* argument = (ConstantDataSequential*) callingArgument;

            Constant *zero = Constant::getNullValue(IntegerType::getInt32Ty(llvmContext));
            vector<llvm::Value*> indices;
            indices.push_back(zero);
            indices.push_back(zero);
            // make global reference to constant
            GlobalVariable *global = new GlobalVariable(*context.getModule(), callingType, true, GlobalValue::PrivateLinkage, argument, ".str");
            // get a pointer to the global constant
            GetElementPtrInst* pointer = GetElementPtrInst::CreateInBounds(callingType, global, makeArrayRef(indices), "", context.currentBlock());
            return pointer;
        }
    }

    cerr << "No valid conversion found for " << getTypeName(callingType) << " to " << getTypeName(declaredType) << endl;
    return nullptr;
}

Value* ProgramNode::generateCode(CodeGenerationContext& context) {
	StatementList::const_iterator it;
	Value *last = nullptr;
	for (it = statements.begin(); it != statements.end(); it++) {
		last = (**it).generateCode(context);
	}
	return last;
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
	if (context.locals().find(name) == context.locals().end()) {
		cerr << "Undeclared variable " << name << endl;
		return nullptr;
	}
	return new LoadInst(context.locals()[name], "", false, context.currentBlock());
}

Value* ReferenceNode::generateCode(CodeGenerationContext& context) {
    const string name = createReferenceName(reference);
	if (context.locals().find(name) == context.locals().end()) {
		cerr << "Undeclared variable " << name << endl;
		return nullptr;
	}
	return new LoadInst(context.locals()[name], "", false, context.currentBlock());
}

Value* FunctionCallNode::generateCode(CodeGenerationContext& context) {
    const string functionName = createReferenceName(reference);
	Function *function = context.getModule()->getFunction(functionName);
	if (function == nullptr) {
		cerr << "No such function: " << functionName << endl;
	}
	vector<Value*> callingArguments;
	ExpressionList::const_iterator callIt = arguments.begin();
    auto declaredIt = function->arg_begin();
	while (callIt != arguments.end()) {
        Argument* declaredArgument = declaredIt;
        Value* callingArgument = (*callIt)->generateCode(context);
        Value* convertedArgument = convertType(declaredArgument, callingArgument, context);
        if (convertedArgument == nullptr) {
            return nullptr;
        }
		callingArguments.push_back(convertedArgument);
        callIt++;
        declaredIt++;
	}
	CallInst *call = CallInst::Create(function, makeArrayRef(callingArguments), "", context.currentBlock());
	return call;
}

Value* BinaryOperatorNode::generateCode(CodeGenerationContext& context) {
	Instruction::BinaryOps instruction;
	switch (operation) {
		case TOKEN_PLUS: 	    instruction = Instruction::Add; goto math;
		case TOKEN_MINUS: 	    instruction = Instruction::Sub; goto math;
		case TOKEN_MULTIPLY:    instruction = Instruction::Mul; goto math;
		case TOKEN_DIVIDE: 	    instruction = Instruction::SDiv; goto math;

		/* TODO: comparison */
	}

	return NULL;
math:
	return BinaryOperator::Create(instruction, leftHandSide.generateCode(context), rightHandSide.generateCode(context), "", context.currentBlock());
}

Value* AssignmentNode::generateCode(CodeGenerationContext& context) {
	if (context.locals().find(leftHandSide.name) == context.locals().end()) {
		cerr << "Undeclared variable " << leftHandSide.name << endl;
		return nullptr;
	}
	return new StoreInst(rightHandSide.generateCode(context), context.locals()[leftHandSide.name], false, context.currentBlock());
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
	context.setCurrentReturnValue(returnValue);
	return returnValue;
}

Value* VariableDeclarationNode::generateCode(CodeGenerationContext& context) {
    LLVMContext& llvmContext = context.getLLVMContext();
    Type* identifierType = typeOf(type, llvmContext);
	AllocaInst *alloc = new AllocaInst(identifierType, identifierType->getPointerAddressSpace(), id.name, context.currentBlock());
	context.locals()[id.name] = alloc;
	if (assignmentExpression != NULL) {
		AssignmentNode assn(id, *assignmentExpression);
		assn.generateCode(context);
	}
	return alloc;
}

Value* FunctionDeclarationNode::generateCode(CodeGenerationContext& context) {
    LLVMContext& llvmContext = context.getLLVMContext();
	vector<Type*> argTypes;
	VariableList::const_iterator it;
	for (it = arguments.begin(); it != arguments.end(); it++) {
		argTypes.push_back(typeOf((**it).type, llvmContext));
	}
	FunctionType *ftype = FunctionType::get(typeOf(type, llvmContext), makeArrayRef(argTypes), false);
    Function *function = nullptr;
    if (id.name == "main") {
        // main function
	    function = Function::Create(ftype, GlobalValue::ExternalLinkage, id.name.c_str(), context.getModule());
        context.setMainFunction(function);
    }
    else {
        // normal function
	    function = Function::Create(ftype, GlobalValue::InternalLinkage, id.name.c_str(), context.getModule());
    }
    BasicBlock *bblock = BasicBlock::Create(llvmContext, "entry", function, 0);

	context.pushBlock(bblock);

	Function::arg_iterator argsValues = function->arg_begin();
    Value* argumentValue;

	for (it = arguments.begin(); it != arguments.end(); it++) {
		(**it).generateCode(context);

		argumentValue = &*argsValues++;
		argumentValue->setName((*it)->id.name.c_str());
        // don't need reference
		new StoreInst(argumentValue, context.locals()[(*it)->id.name], false, bblock);
	}

	block.generateCode(context);
	ReturnInst::Create(llvmContext, context.getCurrentReturnValue(), bblock);

	context.popBlock();
	return function;
}

Value* ImportStatementNode::generateCode(CodeGenerationContext& context) {
    const string packageName = createReferenceName(reference);
    Importer importer;
    importer.importPackage(packageName, context);
    return nullptr;
}

