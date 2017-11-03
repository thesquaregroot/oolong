%option nounput
%option noyywrap
%option yylineno

%{
    #include <string>
    #include "abstract-syntax-tree.h"
    #include "parser.hpp"

    int tokenStart = 1;
    int tokenEnd = 1;

    #define TRACK_TOKEN_LOCATION tokenStart = tokenEnd+1; tokenEnd += yyleng
    #define SAVE_TOKEN yylval.string = new std::string(yytext, yyleng)
    #define TOKEN(t) (yylval.token = t)
%}

%%

\n                      tokenStart = 0; tokenEnd = 0;
[ \t]                   TRACK_TOKEN_LOCATION;
"function"              TRACK_TOKEN_LOCATION; return TOKEN(TOKEN_FUNCTION);
"import"                TRACK_TOKEN_LOCATION; return TOKEN(TOKEN_IMPORT);
"return"                TRACK_TOKEN_LOCATION; return TOKEN(TOKEN_RETURN);
"Boolean"               TRACK_TOKEN_LOCATION; return TOKEN(TOKEN_BOOLEAN);
"Integer"               TRACK_TOKEN_LOCATION; return TOKEN(TOKEN_INTEGER);
"Double"                TRACK_TOKEN_LOCATION; return TOKEN(TOKEN_DOUBLE);
"String"                TRACK_TOKEN_LOCATION; return TOKEN(TOKEN_STRING);
"AND"                   TRACK_TOKEN_LOCATION; return TOKEN(TOKEN_AND);
"OR"                    TRACK_TOKEN_LOCATION; return TOKEN(TOKEN_OR);
"TRUE"                  TRACK_TOKEN_LOCATION; SAVE_TOKEN; return TOKEN_BOOLEAN_LITERAL;
"FALSE"                 TRACK_TOKEN_LOCATION; SAVE_TOKEN; return TOKEN_BOOLEAN_LITERAL;
[a-zA-Z_][a-zA-Z0-9_]*  TRACK_TOKEN_LOCATION; SAVE_TOKEN; return TOKEN_IDENTIFIER;
[0-9]+\.[0-9]*          TRACK_TOKEN_LOCATION; SAVE_TOKEN; return TOKEN_DOUBLE_LITERAL;
[0-9]+                  TRACK_TOKEN_LOCATION; SAVE_TOKEN; return TOKEN_INTEGER_LITERAL;
\"(\\.|[^"\\])*\"       TRACK_TOKEN_LOCATION; SAVE_TOKEN; return TOKEN_STRING_LITERAL;
"="                     TRACK_TOKEN_LOCATION; return TOKEN(TOKEN_ASSIGNMENT);
"=="                    TRACK_TOKEN_LOCATION; return TOKEN(TOKEN_EQUAL_TO);
"!="                    TRACK_TOKEN_LOCATION; return TOKEN(TOKEN_NOT_EQUAL_TO);
"<"                     TRACK_TOKEN_LOCATION; return TOKEN(TOKEN_LESS_THAN);
"<="                    TRACK_TOKEN_LOCATION; return TOKEN(TOKEN_LESS_THAN_OR_EQUAL_TO);
">"                     TRACK_TOKEN_LOCATION; return TOKEN(TOKEN_GREATER_THAN);
">="                    TRACK_TOKEN_LOCATION; return TOKEN(TOKEN_GREATER_THAN_OR_EQUAL_TO);
"("                     TRACK_TOKEN_LOCATION; return TOKEN(TOKEN_LEFT_PARENTHESIS);
")"                     TRACK_TOKEN_LOCATION; return TOKEN(TOKEN_RIGHT_PARENTHESIS);
"{"                     TRACK_TOKEN_LOCATION; return TOKEN(TOKEN_LEFT_BRACE);
"}"                     TRACK_TOKEN_LOCATION; return TOKEN(TOKEN_RIGHT_BRACE);
"."                     TRACK_TOKEN_LOCATION; return TOKEN(TOKEN_PERIOD);
","                     TRACK_TOKEN_LOCATION; return TOKEN(TOKEN_COMMA);
"+"                     TRACK_TOKEN_LOCATION; return TOKEN(TOKEN_PLUS);
"-"                     TRACK_TOKEN_LOCATION; return TOKEN(TOKEN_MINUS);
"*"                     TRACK_TOKEN_LOCATION; return TOKEN(TOKEN_MULTIPLY);
"/"                     TRACK_TOKEN_LOCATION; return TOKEN(TOKEN_DIVIDE);
";"                     TRACK_TOKEN_LOCATION; return TOKEN(TOKEN_SEMICOLON);
":"                     TRACK_TOKEN_LOCATION; return TOKEN(TOKEN_COLON);
.                       printf("Unknown token!\n"); yyterminate();

%%
