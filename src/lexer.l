%{
#include <iostream>
#include <string>
#include "ast.hpp"
#include "parser.hpp" /* Archivo generado por Bison */

// Variables globales
int line_number = 1;
int column_number = 1;
int lexical_errors_count = 0;
std::string parsed_string = "";
%}

/* Opcion para no generar la funcion yywrap */
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
IDENTIFIER      {LETTER}({LETTER}|{DIGIT}|_)*

%%

[ \t\r]+                { column_number += yyleng; /* Ignorar */ }
"\n"                    { line_number++; column_number = 1; }

"//".*                  { column_number += yyleng; /* Ignorar comentarios de una linea */ }

"let"                   { column_number += yyleng; return TOK_LET; }
"in"                    { column_number += yyleng; return TOK_IN; }
"if"                    { column_number += yyleng; return TOK_IF; }
"elif"                  { column_number += yyleng; return TOK_ELIF; }
"else"                  { column_number += yyleng; return TOK_ELSE; }
"while"                 { column_number += yyleng; return TOK_WHILE; }
"for"                   { column_number += yyleng; return TOK_FOR; }
"function"              { column_number += yyleng; return TOK_FUNCTION; }
"type"                  { column_number += yyleng; return TOK_TYPE; }
"inherits"              { column_number += yyleng; return TOK_INHERITS; }
"new"                   { column_number += yyleng; return TOK_NEW; }
"self"                  { column_number += yyleng; return TOK_SELF; }
"base"                  { column_number += yyleng; return TOK_BASE; }
"is"                    { column_number += yyleng; return TOK_IS; }
"as"                    { column_number += yyleng; return TOK_AS; }
"protocol"              { column_number += yyleng; return TOK_PROTOCOL; }
"extends"               { column_number += yyleng; return TOK_EXTENDS; }

"true"                  { column_number += yyleng; return TOK_TRUE; }
"false"                 { column_number += yyleng; return TOK_FALSE; }

"=>"                    { column_number += yyleng; return TOK_ARROW; }
"->"                    { column_number += yyleng; return TOK_TYPE_ARROW; }
":="                    { column_number += yyleng; return TOK_ASSIGN; }
"=="                    { column_number += yyleng; return TOK_EQ; }
"!="                    { column_number += yyleng; return TOK_NEQ; }
"<="                    { column_number += yyleng; return TOK_LEQ; }
">="                    { column_number += yyleng; return TOK_GEQ; }
"@@"                    { column_number += yyleng; return TOK_DCONCAT; }
"@"                     { column_number += yyleng; return TOK_CONCAT; }

{IDENTIFIER}            { column_number += yyleng; yylval.str_val = strdup(yytext); return IDENTIFIER; }
{NUMBER}                { column_number += yyleng; yylval.float_val = atof(yytext); return NUMBER; }

\"                      { column_number += yyleng; parsed_string = ""; BEGIN(STR_STATE); }
<STR_STATE>\"           { column_number += yyleng; BEGIN(INITIAL); yylval.str_val = strdup(parsed_string.c_str()); return STRING; }
<STR_STATE>\\n          { column_number += yyleng; parsed_string += '\n'; }
<STR_STATE>\\\"         { column_number += yyleng; parsed_string += '\"'; }
<STR_STATE>[^\\\"\n]+   { column_number += yyleng; parsed_string += yytext; }
<STR_STATE>\n           { line_number++; column_number = 1; parsed_string += '\n'; }
<STR_STATE>\\t          { column_number += yyleng; parsed_string += '\t'; }

"+"                     { column_number += yyleng; return '+'; }
"-"                     { column_number += yyleng; return '-'; }
"*"                     { column_number += yyleng; return '*'; }
"/"                     { column_number += yyleng; return '/'; }
"%"                     { column_number += yyleng; return '%'; }
"^"                     { column_number += yyleng; return '^'; }
"&"                     { column_number += yyleng; return '&'; }
"|"                     { column_number += yyleng; return '|'; }
"!"                     { column_number += yyleng; return '!'; }
"="                     { column_number += yyleng; return '='; }
"<"                     { column_number += yyleng; return '<'; }
">"                     { column_number += yyleng; return '>'; }
"("                     { column_number += yyleng; return '('; }
")"                     { column_number += yyleng; return ')'; }
"{"                     { column_number += yyleng; return '{'; }
"}"                     { column_number += yyleng; return '}'; }
"["                     { column_number += yyleng; return '['; }
"]"                     { column_number += yyleng; return ']'; }
","                     { column_number += yyleng; return ','; }
"."                     { column_number += yyleng; return '.'; }
":"                     { column_number += yyleng; return ':'; }
";"                     { column_number += yyleng; return ';'; }

.                       {
    std::cerr << "(" << line_number << "," << column_number << ") LEXICAL: unexpected character '"
              << yytext[0] << "'" << std::endl;
    column_number += yyleng;
    lexical_errors_count++;
}

%%