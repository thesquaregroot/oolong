%option nounput

%{
#include <string>
#include "node.h"
#include "parser.hpp"
#define SAVE_TOKEN yylval.string = new std::string(yytext, yyleng)
#define TOKEN(t) (yylval.token = t)
extern "C" int yywrap() { return 0; }
%}

%%

[ \t\n]                 ;
"function"              return TOKEN(TOKEN_FUNCTION);
"return"                return TOKEN(TOKEN_RETURN);
"Boolean"               return TOKEN(TOKEN_BOOLEAN);
"Integer"               return TOKEN(TOKEN_INTEGER);
"Double"                return TOKEN(TOKEN_DOUBLE);
"String"                return TOKEN(TOKEN_STRING);
"AND"                   return TOKEN(TOKEN_AND);
"OR"                    return TOKEN(TOKEN_OR);
"TRUE"                  SAVE_TOKEN; return TOKEN_BOOLEAN_LITERAL;
"FALSE"                 SAVE_TOKEN; return TOKEN_BOOLEAN_LITERAL;
[a-zA-Z_][a-zA-Z0-9_]*  SAVE_TOKEN; return TOKEN_IDENTIFIER;
[0-9]+.[0-9]*           SAVE_TOKEN; return TOKEN_DOUBLE_LITERAL;
[0-9]+                  SAVE_TOKEN; return TOKEN_INTEGER_LITERAL;
\"(\\.|[^"\\])*\"       SAVE_TOKEN; return TOKEN_STRING_LITERAL;
"="                     return TOKEN(TOKEN_ASSIGNMENT);
"=="                    return TOKEN(TOKEN_EQUAL_TO);
"!="                    return TOKEN(TOKEN_NOT_EQUAL_TO);
"<"                     return TOKEN(TOKEN_LESS_THAN);
"<="                    return TOKEN(TOKEN_LESS_THAN_OR_EQUAL_TO);
">"                     return TOKEN(TOKEN_GREATER_THAN);
">="                    return TOKEN(TOKEN_GREATER_THAN_OR_EQUAL_TO);
"("                     return TOKEN(TOKEN_LEFT_PARENTHESIS);
")"                     return TOKEN(TOKEN_RIGHT_PARENTHESIS);
"{"                     return TOKEN(TOKEN_LEFT_BRACE);
"}"                     return TOKEN(TOKEN_RIGHT_BRACE);
"."                     return TOKEN(TOKEN_DOT);
","                     return TOKEN(TOKEN_COMMA);
"+"                     return TOKEN(TOKEN_PLUS);
"-"                     return TOKEN(TOKEN_MINUS);
"*"                     return TOKEN(TOKEN_MULTIPLY);
"/"                     return TOKEN(TOKEN_DIVIDE);
";"                     return TOKEN(TOKEN_SEMICOLON);
":"                     return TOKEN(TOKEN_COLON);
.                       printf("Unknown token!\n"); yyterminate();

%%
