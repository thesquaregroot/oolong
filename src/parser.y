%define parse.error verbose
%define parse.trace true

%{
    #define YYDEBUG 1
    #include "abstract-syntax-tree.h"
    #include "tokens.h"
    #include "common.h"
    #include <stdio.h>

    BlockNode* programNode; /* the top level root node of our final AST */

    extern int yylex();
    extern int tokenStart;
    extern int tokenEnd;
    extern const char* currentParseFile;

    void yyerror(const char *s) {
        fprintf(stderr, "ERROR: In file %s (line %d: %d-%d): %s\n", currentParseFile, yylineno, tokenStart, tokenEnd, s);
    }

    std::string interpretString(const std::string& input) {
        // remove leading and trailing quotes
        std::string value = input.substr(1, input.length()-2);
        // un-escape any other quotes
        replaceAll(value, "\\\"", "\"");
        replaceAll(value, "\\n", "\n");
        replaceAll(value, "\\r", "\r");
        return value;
    }
%}

/* Represents the many different ways we can access our data */
%union {
    Node *node;
    BlockNode *block;
    StatementList *statementList;
    ExpressionNode *expression;
    IfStatementNode *ifStatement;
    StatementNode *statement;
    AssignableNode *assignable;
    IdentifierNode *identifier;
    IdentifierList *identifierList;
    VariableDeclarationNode *variableDeclaration;
    std::vector<VariableDeclarationNode*> *variableDeclarationList;
    std::vector<ExpressionNode*> *expressionList;
    std::string *string;
    int token;
}

/* Define our terminal symbols (tokens). This should
   match our tokens.l lex file. We also define the node type
   they represent.
 */
%token <string> TOKEN_IDENTIFIER
%token <string> TOKEN_INTEGER TOKEN_DOUBLE TOKEN_STRING TOKEN_BOOLEAN
%token <string> TOKEN_INTEGER_LITERAL TOKEN_DOUBLE_LITERAL TOKEN_STRING_LITERAL TOKEN_BOOLEAN_LITERAL
%token <token> TOKEN_FUNCTION TOKEN_IMPORT TOKEN_RETURN TOKEN_AND TOKEN_OR
%token <token> TOKEN_EQUAL_TO TOKEN_NOT_EQUAL_TO TOKEN_LESS_THAN TOKEN_LESS_THAN_OR_EQUAL_TO TOKEN_GREATER_THAN TOKEN_GREATER_THAN_OR_EQUAL_TO
%token <token> TOKEN_ASSIGNMENT TOKEN_SEMICOLON TOKEN_COLON TOKEN_COMMA TOKEN_PERIOD
%token <token> TOKEN_LEFT_PARENTHESIS TOKEN_RIGHT_PARENTHESIS TOKEN_LEFT_BRACE TOKEN_RIGHT_BRACE
%token <token> TOKEN_PLUS TOKEN_MINUS TOKEN_MULTIPLY TOKEN_DIVIDE TOKEN_PERCENT
%token <token> TOKEN_IF TOKEN_ELSE TOKEN_NOT TOKEN_WHILE
%token <token> TOKEN_ADD_ASSIGN TOKEN_SUBTRACT_ASSIGN TOKEN_MULTIPLY_ASSIGN TOKEN_DIVIDE_ASSIGN TOKEN_MODULO_ASSIGN
%token <token> TOKEN_INCREMENT TOKEN_DECREMENT

/* Define the type of node our nonterminal symbols represent.
   The types refer to the %union declaration above. Ex: when
   we call an ident (defined by union type ident) we are really
   calling an (IdentifierNode*). It makes the compiler happy.
 */
%type <block> block
%type <statementList> program statement_list
%type <expression> expression literal_value
%type <ifStatement> else_list
%type <statement> statement function_declaration assignment_statement
%type <assignable> assignable
%type <identifier> identifier type
%type <identifierList> reference
%type <variableDeclaration> variable_declaration
%type <variableDeclarationList> function_declaration_argument_list
%type <expressionList> function_call_argument_list

/* Operator precedence for mathematical operators */
%left TOKEN_OR
%left TOKEN_AND
%left TOKEN_EQUAL_TO TOKEN_NOT_EQUAL_TO
%nonassoc TOKEN_LESS_THAN TOKEN_LESS_THAN_OR_EQUAL_TO TOKEN_GREATER_THAN TOKEN_GREATER_THAN_OR_EQUAL_TO
%left TOKEN_PLUS TOKEN_MINUS
%left TOKEN_MULTIPLY TOKEN_DIVIDE TOKEN_PERCENT
%right TOKEN_NOT

%start program

%%

program : statement_list
            {
                programNode = new BlockNode();
                programNode->statements = *$1;
            }
        ;

statement_list : statement
                    {
                        $$ = new StatementList();
                        $$->push_back($<statement>1);
                    }
               | statement_list statement
                    {
                        $$ = $1;
                        $1->push_back($<statement>2);
                    }
               ;

statement : TOKEN_IMPORT reference TOKEN_SEMICOLON
                {
                    $$ = new ImportStatementNode(*$2);
                }
          | function_declaration
          | variable_declaration TOKEN_SEMICOLON
                {
                    $$ = $1;
                }
          | variable_declaration TOKEN_ASSIGNMENT expression TOKEN_SEMICOLON
                {
                    $1->assignmentExpression = $3;
                    $$ = $1;
                }
          | assignment_statement
          | expression TOKEN_SEMICOLON
                {
                    $$ = new ExpressionStatementNode(*$1);
                }
          | TOKEN_RETURN expression TOKEN_SEMICOLON
                {
                    $$ = new ReturnStatementNode(*$2);
                }
          | block
                {
                    $$ = $1;
                }
          | TOKEN_IF TOKEN_LEFT_PARENTHESIS expression TOKEN_RIGHT_PARENTHESIS block
                {
                    $$ = new IfStatementNode($3, *$5);
                }
          | TOKEN_IF TOKEN_LEFT_PARENTHESIS expression TOKEN_RIGHT_PARENTHESIS block else_list
                {
                    $$ = new IfStatementNode($3, *$5, $6);
                }
          | TOKEN_WHILE TOKEN_LEFT_PARENTHESIS expression TOKEN_RIGHT_PARENTHESIS block
                {
                    $$ = new WhileLoopNode($3, *$5);
                }
          ;

reference : identifier
            {
                $$ = new IdentifierList();
                $$->push_back($1);
            }
          | reference TOKEN_PERIOD identifier
            {
                $$ = $1;
                $$->push_back($3);
            }
          ;

function_declaration : TOKEN_FUNCTION identifier TOKEN_LEFT_PARENTHESIS function_declaration_argument_list TOKEN_RIGHT_PARENTHESIS TOKEN_COLON type block
                        {
                            $$ = new FunctionDeclarationNode(*$7, *$2, *$4, *$8);
                        }
                     ;

block : TOKEN_LEFT_BRACE statement_list TOKEN_RIGHT_BRACE
            {
                $$ = new BlockNode();
                $$->statements = *$2;
            }
      ;

else_list : TOKEN_ELSE block
                {
                    $$ = new IfStatementNode(nullptr, *$2);
                }
          | else_if TOKEN_LEFT_PARENTHESIS expression TOKEN_RIGHT_PARENTHESIS block
                {
                    $$ = new IfStatementNode($3, *$5);
                }
          | else_if TOKEN_LEFT_PARENTHESIS expression TOKEN_RIGHT_PARENTHESIS block else_list
                {
                    $$ = new IfStatementNode($3, *$5, $6);
                }
          ;

else_if : TOKEN_ELSE
        | TOKEN_ELSE TOKEN_IF
        ;

assignment_statement : assignable TOKEN_ASSIGNMENT expression TOKEN_SEMICOLON
                        {
                            $$ = new AssignmentNode(*$1, *$3);
                        }
                     | assignable TOKEN_ADD_ASSIGN expression TOKEN_SEMICOLON
                        {
                            auto variableReference = new ReferenceNode(*$1);
                            auto calculation = new BinaryOperatorNode(*variableReference, (int)TOKEN_PLUS, *$3);
                            $$ = new AssignmentNode(*$1, *calculation);
                        }
                     | assignable TOKEN_SUBTRACT_ASSIGN expression TOKEN_SEMICOLON
                        {
                            auto variableReference = new ReferenceNode(*$1);
                            auto calculation = new BinaryOperatorNode(*variableReference, (int)TOKEN_MINUS, *$3);
                            $$ = new AssignmentNode(*$1, *calculation);
                        }
                     | assignable TOKEN_MULTIPLY_ASSIGN expression TOKEN_SEMICOLON
                        {
                            auto variableReference = new ReferenceNode(*$1);
                            auto calculation = new BinaryOperatorNode(*variableReference, (int)TOKEN_MULTIPLY, *$3);
                            $$ = new AssignmentNode(*$1, *calculation);
                        }
                     | assignable TOKEN_DIVIDE_ASSIGN expression TOKEN_SEMICOLON
                        {
                            auto variableReference = new ReferenceNode(*$1);
                            auto calculation = new BinaryOperatorNode(*variableReference, (int)TOKEN_DIVIDE, *$3);
                            $$ = new AssignmentNode(*$1, *calculation);
                        }
                     | assignable TOKEN_MODULO_ASSIGN expression TOKEN_SEMICOLON
                        {
                            auto variableReference = new ReferenceNode(*$1);
                            auto calculation = new BinaryOperatorNode(*variableReference, (int)TOKEN_PERCENT, *$3);
                            $$ = new AssignmentNode(*$1, *calculation);
                        }
                     ;

assignable : identifier
                {
                    $$ = new AssignableNode(*$1);
                }
           ;

variable_declaration : identifier TOKEN_COLON type
                        {
                            $$ = new VariableDeclarationNode(*$3, *$1);
                        }
                     ;

    
function_declaration_argument_list : %empty
                                        {
                                            $$ = new VariableList();
                                        }
                                   | variable_declaration
                                        {
                                            $$ = new VariableList();
                                            $$->push_back($1);
                                        }
                                   | function_declaration_argument_list TOKEN_COMMA variable_declaration
                                        {
                                            $1->push_back($3);
                                        }
                                   ;

identifier : TOKEN_IDENTIFIER
                {
                    $$ = new IdentifierNode(*$1);
                    delete $1;
                }
           ;

literal_value : TOKEN_BOOLEAN_LITERAL
                    {
                        $$ = new BooleanNode(*$1 == "true");
                    }
              | TOKEN_INTEGER_LITERAL
                    {
                        $$ = new IntegerNode(atol($1->c_str()));
                        delete $1;
                    }
              | TOKEN_DOUBLE_LITERAL
                    {
                        $$ = new DoubleNode(atof($1->c_str()));
                        delete $1;
                    }
              | TOKEN_STRING_LITERAL
                    {
                        const std::string value = interpretString(*$1);
                        // create node with actual string value
                        $$ = new StringNode(value);
                    }
              ;
    
expression : reference TOKEN_LEFT_PARENTHESIS TOKEN_RIGHT_PARENTHESIS
                {
                    $$ = new FunctionCallNode(*$1);
                }
           | reference TOKEN_LEFT_PARENTHESIS function_call_argument_list TOKEN_RIGHT_PARENTHESIS
                {
                    $$ = new FunctionCallNode(*$1, *$3);
                }
           | reference
                {
                    $$ = new ReferenceNode(*$1);
                }
           | literal_value
           | TOKEN_LEFT_PARENTHESIS expression TOKEN_RIGHT_PARENTHESIS
                {
                    $$ = $2;
                }
           | expression TOKEN_PLUS expression
                {
                    $$ = new BinaryOperatorNode(*$1, $2, *$3);
                }
           | expression TOKEN_MINUS expression
                {
                    $$ = new BinaryOperatorNode(*$1, $2, *$3);
                }
           | expression TOKEN_MULTIPLY expression
                {
                    $$ = new BinaryOperatorNode(*$1, $2, *$3);
                }
           | expression TOKEN_DIVIDE expression
                {
                    $$ = new BinaryOperatorNode(*$1, $2, *$3);
                }
           | expression TOKEN_PERCENT expression
                {
                    $$ = new BinaryOperatorNode(*$1, $2, *$3);
                }
           | expression TOKEN_EQUAL_TO expression
                {
                    $$ = new BinaryOperatorNode(*$1, $2, *$3);
                }
           | expression TOKEN_NOT_EQUAL_TO expression
                {
                    $$ = new BinaryOperatorNode(*$1, $2, *$3);
                }
           | expression TOKEN_LESS_THAN expression
                {
                    $$ = new BinaryOperatorNode(*$1, $2, *$3);
                }
           | expression TOKEN_LESS_THAN_OR_EQUAL_TO expression
                {
                    $$ = new BinaryOperatorNode(*$1, $2, *$3);
                }
           | expression TOKEN_GREATER_THAN expression
                {
                    $$ = new BinaryOperatorNode(*$1, $2, *$3);
                }
           | expression TOKEN_GREATER_THAN_OR_EQUAL_TO expression
                {
                    $$ = new BinaryOperatorNode(*$1, $2, *$3);
                }
           | expression TOKEN_AND expression
                {
                    $$ = new BinaryOperatorNode(*$1, $2, *$3);
                }
           | expression TOKEN_OR expression
                {
                    $$ = new BinaryOperatorNode(*$1, $2, *$3);
                }
           | TOKEN_MINUS expression
                {
                    $$ = new UnaryOperatorNode($1, *$2);
                }
           | TOKEN_NOT expression
                {
                    $$ = new UnaryOperatorNode($1, *$2);
                }
           | assignable TOKEN_INCREMENT
                {
                    $$ = new IncrementExpressionNode(*$1, true);
                }
           | TOKEN_INCREMENT assignable
                {
                    $$ = new IncrementExpressionNode(*$2, false);
                }
           | assignable TOKEN_DECREMENT
                {
                    $$ = new DecrementExpressionNode(*$1, true);
                }
           | TOKEN_DECREMENT assignable
                {
                    $$ = new DecrementExpressionNode(*$2, false);
                }
           ;

function_call_argument_list : expression
                                {
                                    $$ = new ExpressionList();
                                    $$->push_back($1);
                                }
                            | function_call_argument_list TOKEN_COMMA expression
                                {
                                    $$ = $1;
                                    $$->push_back($3);
                                }
                            ;

type : TOKEN_BOOLEAN
        {
            $$ = new IdentifierNode("Boolean");
        }
     | TOKEN_INTEGER
        {
            $$ = new IdentifierNode("Integer");
        }
     | TOKEN_DOUBLE
        {
            $$ = new IdentifierNode("Double");
        }
     | TOKEN_STRING
        {
            $$ = new IdentifierNode("String");
        }
     ;

%%
