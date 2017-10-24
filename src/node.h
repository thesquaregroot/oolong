#ifndef NODE_H
#define NODE_H

#include <iostream>
#include <vector>
#include <llvm/IR/Value.h>

class CodeGenerationContext;
class StatementNode;
class ExpressionNode;
class VariableDeclarationNode;

typedef std::vector<StatementNode*> StatementList;
typedef std::vector<ExpressionNode*> ExpressionList;
typedef std::vector<VariableDeclarationNode*> VariableList;

class Node {
public:
    virtual ~Node() {}
    virtual llvm::Value* generateCode(CodeGenerationContext& context) { return nullptr; }
};

class ProgramNode : public Node {
public:
    ProgramNode() {}

    StatementList* statements;

    virtual llvm::Value* generateCode(CodeGenerationContext& context);
};

class ExpressionNode : public Node {
};

class StatementNode : public Node {
};

class BooleanNode : public ExpressionNode {
public:
    BooleanNode(bool value) : value(value) {}

    bool value;

    virtual llvm::Value* generateCode(CodeGenerationContext& context);
};

class IntegerNode : public ExpressionNode {
public:
    IntegerNode(long long value) : value(value) {}

    long long value;

    virtual llvm::Value* generateCode(CodeGenerationContext& context);
};

class DoubleNode : public ExpressionNode {
public:
    DoubleNode(double value) : value(value) {}

    double value;

    virtual llvm::Value* generateCode(CodeGenerationContext& context);
};

class StringNode : public ExpressionNode {
public:
    StringNode(std::string& value) : value(value) {}

    std::string value;

    virtual llvm::Value* generateCode(CodeGenerationContext& context);
};

class IdentifierNode : public ExpressionNode {
public:
    IdentifierNode(const std::string& name) : name(name) {}

    std::string name;

    virtual llvm::Value* generateCode(CodeGenerationContext& context);
};

class FunctionCallNode : public ExpressionNode {
public:
    FunctionCallNode(const IdentifierNode& id, ExpressionList& arguments) : id(id), arguments(arguments) {}
    FunctionCallNode(const IdentifierNode& id) : id(id) {}

    const IdentifierNode& id;
    ExpressionList arguments;

    virtual llvm::Value* generateCode(CodeGenerationContext& context);
};

class BinaryOperatorNode : public ExpressionNode {
public:
    BinaryOperatorNode(ExpressionNode& leftHandSide, int operation, ExpressionNode& rightHandSide) : leftHandSide(leftHandSide), operation(operation), rightHandSide(rightHandSide) {}

    ExpressionNode& leftHandSide;
    int operation;
    ExpressionNode& rightHandSide;

    virtual llvm::Value* generateCode(CodeGenerationContext& context);
};

class AssignmentNode : public StatementNode {
public:
    AssignmentNode(IdentifierNode& leftHandSide, ExpressionNode& rightHandSide) : leftHandSide(leftHandSide), rightHandSide(rightHandSide) {}

    IdentifierNode& leftHandSide;
    ExpressionNode& rightHandSide;

    virtual llvm::Value* generateCode(CodeGenerationContext& context);
};

class BlockNode : public StatementNode {
public:
    BlockNode() {}

    StatementList statements;

    virtual llvm::Value* generateCode(CodeGenerationContext& context);
};

class ExpressionStatementNode : public StatementNode {
public:
    ExpressionStatementNode(ExpressionNode& expression) : expression(expression) {}

    ExpressionNode& expression;

    virtual llvm::Value* generateCode(CodeGenerationContext& context);
};

class ReturnStatementNode : public StatementNode {
public:
    ReturnStatementNode(ExpressionNode& expression) : expression(expression) {}

    ExpressionNode& expression;

    virtual llvm::Value* generateCode(CodeGenerationContext& context);
};

class VariableDeclarationNode : public StatementNode {
public:
    VariableDeclarationNode(const IdentifierNode& type, IdentifierNode& id) : type(type), id(id) {}
    VariableDeclarationNode(const IdentifierNode& type, IdentifierNode& id, ExpressionNode *assignmentExpression) : type(type), id(id), assignmentExpression(assignmentExpression) {}

    const IdentifierNode& type;
    IdentifierNode& id;
    ExpressionNode *assignmentExpression;

    virtual llvm::Value* generateCode(CodeGenerationContext& context);
};

class FunctionDeclarationNode : public StatementNode {
public:
    FunctionDeclarationNode(const IdentifierNode& type, const IdentifierNode& id, const VariableList& arguments, BlockNode& block) : type(type), id(id), arguments(arguments), block(block) {}

    const IdentifierNode& type;
    const IdentifierNode& id;
    VariableList arguments;
    BlockNode& block;

    virtual llvm::Value* generateCode(CodeGenerationContext& context);
};

#endif

