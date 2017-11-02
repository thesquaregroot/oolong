%define parse.error verbose
%define parse.trace true

%{
    #define YYDEBUG 1
    #include "abstract-syntax-tree.h"
    #include <stdio.h>
    ProgramNode *programNode; /* the top level root node of our final AST */

    extern int yylex();
    void yyerror(const char *s) {
        printf("ERROR: %s\n", s);
    }

    void replaceAll(std::string& value, const std::string substring, const std::string replacement) {
        size_t index = value.find(substring);
        while (index != std::string::npos) {
            // replace \" sequence with just "
            value.replace(index, substring.length(), replacement);
            // re-search starting at end of replacement
            index = value.find("\\\"", index + replacement.length() - 1);
        }
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
    StatementNode *statement;
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
%token <token> TOKEN_EQUAL_TO TOKEN_NOT_EQUAL_TO TOKEN_LESS_THAN
%token <token> TOKEN_LESS_THAN_OR_EQUAL_TO TOKEN_GREATER_THAN
%token <token> TOKEN_GREATER_THAN_OR_EQUAL_TO TOKEN_ASSIGNMENT
%token <token> TOKEN_LEFT_PARENTHESIS TOKEN_RIGHT_PARENTHESIS
%token <token> TOKEN_LEFT_BRACE TOKEN_RIGHT_BRACE TOKEN_COMMA
%token <token> TOKEN_PERIOD TOKEN_PLUS TOKEN_MINUS TOKEN_MULTIPLY
%token <token> TOKEN_DIVIDE TOKEN_SEMICOLON TOKEN_COLON

/* Define the type of node our nonterminal symbols represent.
   The types refer to the %union declaration above. Ex: when
   we call an ident (defined by union type ident) we are really
   calling an (IdentifierNode*). It makes the compiler happy.
 */
%type <block> block
%type <statementList> program statement_list
%type <expression> expression literal_value
%type <statement> statement function_declaration
%type <identifier> identifier type
%type <identifierList> reference
%type <variableDeclaration> variable_declaration
%type <variableDeclarationList> function_declaration_argument_list
%type <expressionList> function_call_argument_list

/* Operator precedence for mathematical operators */
%left TOKEN_PLUS TOKEN_MINUS
%left TOKEN_MULTIPLY TOKEN_DIVIDE
%nonassoc TOKEN_EQUAL_TO TOKEN_NOT_EQUAL_TO TOKEN_LESS_THAN
%nonassoc TOKEN_LESS_THAN_OR_EQUAL_TO TOKEN_GREATER_THAN
%nonassoc TOKEN_GREATER_THAN_OR_EQUAL_TO

%start program

%%

program : statement_list
            {
                programNode = new ProgramNode();
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
                {
                    $$ = $1;
                }
          | variable_declaration TOKEN_SEMICOLON
                {
                    $$ = $1;
                }
          | variable_declaration TOKEN_ASSIGNMENT expression TOKEN_SEMICOLON
                {
                    $1->assignmentExpression = $3;
                    $$ = $1;
                }
          | identifier TOKEN_ASSIGNMENT expression TOKEN_SEMICOLON
                {
                    $$ = new AssignmentNode(*$1, *$3);
                }
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
                    $$ = new IfStatementNode($3, $5);
                }
          | TOKEN_IF TOKEN_LEFT_PARENTHESIS expression TOKEN_RIGHT_PARENTHESIS block else-list
                {
                    $$ = new IfStatementNode($3, $5, $6);
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
                            delete $4;
                        }
                     ;

block : TOKEN_LEFT_BRACE statement_list TOKEN_RIGHT_BRACE
            {
                $$ = new BlockNode();
                $$->statements = *$2;
            }
      ;

else_list : %empty
          | TOKEN_ELSE block
          | TOKEN ELSE TOKEN_LEFT_PARENTHESIS expression TOKEN_RIGHT_PARENTHESIS block else_list

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
                        $$ = new BooleanNode(*$1 == "TRUE");
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
    
expression : reference TOKEN_LEFT_PARENTHESIS function_call_argument_list TOKEN_RIGHT_PARENTHESIS
                {
                    $$ = new FunctionCallNode(*$1, *$3);
                    delete $3;
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
           ;
    
function_call_argument_list : %empty
                                {
                                    $$ = new ExpressionList();
                                }
                            | expression
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
