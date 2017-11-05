#ifndef ABSTRACT_SYNTAX_TREE_H
#define ABSTRACT_SYNTAX_TREE_H

#include <string>
#include <iostream>
#include <vector>
#include <llvm/IR/Value.h>

class CodeGenerationContext;
class StatementNode;
class ExpressionNode;
class VariableDeclarationNode;
class IdentifierNode;

typedef std::vector<StatementNode*> StatementList;
typedef std::vector<ExpressionNode*> ExpressionList;
typedef std::vector<VariableDeclarationNode*> VariableList;
typedef std::vector<IdentifierNode*> IdentifierList;

class Node {
public:
    virtual ~Node() {}

    int lineNumber;

    virtual llvm::Value* generateCode(CodeGenerationContext& context) { return nullptr; }
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
    StringNode(const std::string& value) : value(value) {}

    std::string value;

    virtual llvm::Value* generateCode(CodeGenerationContext& context);
};

class IdentifierNode : public ExpressionNode {
public:
    IdentifierNode(const std::string& name) : name(name) {}

    std::string name;

    virtual llvm::Value* generateCode(CodeGenerationContext& context);
};


class ReferenceNode : public ExpressionNode {
public:
    ReferenceNode(const IdentifierList& reference) : reference(reference) {}

    const IdentifierList& reference;

    virtual llvm::Value* generateCode(CodeGenerationContext& context);
};

class FunctionCallNode : public ExpressionNode {
public:
    FunctionCallNode(const IdentifierList& reference, ExpressionList& arguments) : reference(reference), arguments(arguments) {}
    FunctionCallNode(const IdentifierList& reference) : reference(reference) {}

    const IdentifierList& reference;
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
    FunctionDeclarationNode(const IdentifierNode& type, const IdentifierNode& id, VariableList& arguments, BlockNode& block) : type(type), id(id), arguments(arguments), block(block) {}

    const IdentifierNode& type;
    const IdentifierNode& id;
    VariableList& arguments;
    BlockNode& block;

    virtual llvm::Value* generateCode(CodeGenerationContext& context);
};

class ImportStatementNode : public StatementNode {
public:
    ImportStatementNode(const IdentifierList& reference) : reference(reference) {}

    const IdentifierList& reference;

    virtual llvm::Value* generateCode(CodeGenerationContext& context);
};

class IfStatementNode : public StatementNode {
public:
    IfStatementNode(ExpressionNode* condition, BlockNode& block) : condition(condition), block(block) {}
    IfStatementNode(ExpressionNode* condition, BlockNode& block, IfStatementNode* elseStatement) : condition(condition), block(block), elseStatement(elseStatement) {}

    ExpressionNode* condition;
    BlockNode& block;
    IfStatementNode* elseStatement;

    virtual llvm::Value* generateCode(CodeGenerationContext& context);
};

#endif
