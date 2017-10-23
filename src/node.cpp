#include "node.h"
#include "code-generation.h"
#include "parser.hpp"

using namespace std;
using namespace llvm;

Value* IntegerNode::generateCode(CodeGenerationContext& context)
{
    std::cout << "Creating integer: " << value << std::endl;
    return ConstantInt::get(Type::getInt64Ty(getGlobalContext()), value, true);
}

Value* DoubleNode::generateCode(CodeGenerationContext& context)
{
    std::cout << "Creating double: " << value << std::endl;
    return ConstantFP::get(Type::getDoubleTy(getGlobalContext()), value);
}

Value* IdentifierNode::generateCode(CodeGenerationContext& context)
{
    std::cout << "Creating identifier reference: " << name << std::endl;
    if (context.locals().find(name) == context.locals().end()) {
        std::cerr << "undeclared variable " << name << std::endl;
        return NULL;
    }
    return new LoadInst(context.locals()[name], "", false, context.currentBlock());
}

Value* MethodCallNode::generateCode(CodeGenerationContext& context)
{
    Function *function = context.module->getFunction(id.name.c_str());
    if (function == NULL) {
        std::cerr << "no such function " << id.name << std::endl;
    }
    std::vector<Value*> args;
    ExpressionList::const_iterator it;
    for (it = arguments.begin(); it != arguments.end(); it++) {
        args.push_back((**it).generateCode(context));
    }
    CallInst *call = CallInst::Create(function, args.begin(), args.end(), "", context.currentBlock());
    std::cout << "Creating method call: " << id.name << std::endl;
    return call;
}

Value* BinaryOperatorNode::generateCode(CodeGenerationContext& context)
{
    std::cout << "Creating binary operation " << op << std::endl;
    Instruction::BinaryOps instr;
    switch (op) {
        case TPLUS:     instr = Instruction::Add; goto math;
        case TMIUSNode:    instr = Instruction::Sub; goto math;
        case TMUL:      instr = Instruction::Mul; goto math;
        case TDIV:      instr = Instruction::SDiv; goto math;

        /* TODO comparison */
    }

    return NULL;
math:
    return BinaryOperator::Create(instr, lhs.generateCode(context),
        rhs.generateCode(context), "", context.currentBlock());
}

Value* AssignmentNode::generateCode(CodeGenerationContext& context)
{
    std::cout << "Creating assignment for " << lhs.name << std::endl;
    if (context.locals().find(lhs.name) == context.locals().end()) {
        std::cerr << "undeclared variable " << lhs.name << std::endl;
        return NULL;
    }
    return new StoreInst(rhs.generateCode(context), context.locals()[lhs.name], false, context.currentBlock());
}

Value* BlockNode::generateCode(CodeGenerationContext& context)
{
    StatementList::const_iterator it;
    Value *last = NULL;
    for (it = statements.begin(); it != statements.end(); it++) {
        std::cout << "Generating code for " << typeid(**it).name() << std::endl;
        last = (**it).generateCode(context);
    }
    std::cout << "Creating block" << std::endl;
    return last;
}

Value* ExpressionStatementNode::generateCode(CodeGenerationContext& context)
{
    std::cout << "Generating code for " << typeid(expression).name() << std::endl;
    return expression.generateCode(context);
}

Value* VariableDeclarationNode::generateCode(CodeGenerationContext& context)
{
    std::cout << "Creating variable declaration " << type.name << " " << id.name << std::endl;
    AllocaInst *alloc = new AllocaInst(typeOf(type), id.name.c_str(), context.currentBlock());
    context.locals()[id.name] = alloc;
    if (assignmentExpr != NULL) {
        AssignmentNode assn(id, *assignmentExpr);
        assn.generateCode(context);
    }
    return alloc;
}

Value* FunctionDeclarationNode::generateCode(CodeGenerationContext& context)
{
    vector<const Type*> argTypes;
    VariableList::const_iterator it;
    for (it = arguments.begin(); it != arguments.end(); it++) {
        argTypes.push_back(typeOf((**it).type));
    }
    FunctionType *ftype = FunctionType::get(typeOf(type), argTypes, false);
    Function *function = Function::Create(ftype, GlobalValue::InternalLinkage, id.name.c_str(), context.module);
    BasicBlock *bblock = BasicBlock::Create(getGlobalContext(), "entry", function, 0);

    context.pushBlock(bblock);

    for (it = arguments.begin(); it != arguments.end(); it++) {
        (**it).generateCode(context);
    }

    block.generateCode(context);
    ReturnInst::Create(getGlobalContext(), bblock);

    context.popBlock();
    std::cout << "Creating function: " << id.name << std::endl;
    return function;
}

