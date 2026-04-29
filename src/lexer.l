%{
#include <iostream>
#include <string>
#include "ast.hpp"
#include "parser.hpp" /* Archivo generado por Bison */

// Variables globales
int line_number = 1;
std::string parsed_string = "";
%}

/* Opción para no generar la función yywrap */
%option noyywrap
/* Evita advertencias de funciones no utilizadas */
%option nounput
%option noinput

/* Estado exclusivo para Strings */
%x STR_STATE

/* Definiciones de expresiones regulares */
DIGIT           [0-9]
LETTER          [a-zA-Z]
NUMBER          {DIGIT}+(\.{DIGIT}+)?|\.{DIGIT}+
IDENTIFIER      ({LETTER}|_)({LETTER}|{DIGIT}|_)*

%%

[ \t\r]+                { /* Ignorar */ }
"\n"                    { line_number++; }

"//".* { /* Ignorar comentarios */ }

"let"                   { return TOK_LET; }
"in"                    { return TOK_IN; }
"if"                    { return TOK_IF; }
"elif"                  { return TOK_ELIF; }
"else"                  { return TOK_ELSE; }
"while"                 { return TOK_WHILE; }
"for"                   { return TOK_FOR; }
"function"              { return TOK_FUNCTION; }
"type"                  { return TOK_TYPE; }
"inherits"              { return TOK_INHERITS; }
"new"                   { return TOK_NEW; }
"self"                  { return TOK_SELF; }
"base"                  { return TOK_BASE; }
"is"                    { return TOK_IS; }
"as"                    { return TOK_AS; }
"protocol"              { return TOK_PROTOCOL; }
"extends"               { return TOK_EXTENDS; }

"true"                  { return TOK_TRUE; }
"false"                 { return TOK_FALSE; }

"=>"                    { return TOK_ARROW; }
"->"                    { return TOK_TYPE_ARROW; }
":="                    { return TOK_ASSIGN; }
"=="                    { return TOK_EQ; }
"!="                    { return TOK_NEQ; }
"<="                    { return TOK_LEQ; }
">="                    { return TOK_GEQ; }
"@@"                    { return TOK_DCONCAT; }
"@"                     { return TOK_CONCAT; }

{IDENTIFIER}            { yylval.str_val = strdup(yytext); return IDENTIFIER; }
{NUMBER}                { yylval.float_val = atof(yytext); return NUMBER; }

\"                      { parsed_string = ""; BEGIN(STR_STATE); }
<STR_STATE>\"           { BEGIN(INITIAL); yylval.str_val = strdup(parsed_string.c_str()); return STRING; }
<STR_STATE>\\n          { parsed_string += '\n'; }
<STR_STATE>\\\"         { parsed_string += '\"'; }
<STR_STATE>[^\\\"\n]+   { parsed_string += yytext; }
<STR_STATE>\n           { line_number++; parsed_string += '\n'; }

"+"                     { return '+'; }
"-"                     { return '-'; }
"*"                     { return '*'; }
"/"                     { return '/'; }
"%"                     { return '%'; }
"^"                     { return '^'; }
"&"                     { return '&'; }
"|"                     { return '|'; }
"!"                     { return '!'; }
"="                     { return '='; }
"<"                     { return '<'; }
">"                     { return '>'; }
"("                     { return '('; }
")"                     { return ')'; }
"{"                     { return '{'; }
"}"                     { return '}'; }
"["                     { return '['; }
"]"                     { return ']'; }
","                     { return ','; }
"."                     { return '.'; }
":"                     { return ':'; }
";"                     { return ';'; }

.                       { 
    std::cerr << "Lexical Error: Caracter inesperado '" << yytext[0] 
              << "' (ASCII: " << (int)yytext[0] << ") en linea " << line_number << std::endl; 
}

%%