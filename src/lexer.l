%{
#include <iostream>
#include <string>
#include "ast.hpp"

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

"let"                   { return 100; }
"in"                    { return 101; }
"if"                    { return 102; }
"elif"                  { return 103; }
"else"                  { return 104; }
"while"                 { return 105; }
"for"                   { return 106; }
"function"              { return 107; }
"type"                  { return 108; }
"inherits"              { return 109; }
"new"                   { return 110; }
"self"                  { return 111; }
"base"                  { return 112; }
"is"                    { return 113; }
"as"                    { return 114; }
"protocol"              { return 115; }
"extends"               { return 116; }

"true"                  { return 117; }
"false"                 { return 118; }

"=>"                    { return 200; }
"->"                    { return 201; }
":="                    { return 202; }

"=="                    { return 210; }
"!="                    { return 211; }
"<="                    { return 212; }
">="                    { return 213; }
"<"                     { return '<'; }
">"                     { return '>'; }

"@@"                    { return 220; }
"@"                     { return '@'; }

"+"                     { return '+'; }
"-"                     { return '-'; }
"*"                     { return '*'; }
"/"                     { return '/'; }
"^"                     { return '^'; }
"&"                     { return '&'; }
"|"                     { return '|'; }
"!"                     { return '!'; }
"="                     { return '='; }

{IDENTIFIER}            { return 4; }
{NUMBER}                { return 2; }

\"                      { parsed_string = ""; BEGIN(STR_STATE); }
<STR_STATE>\"           { BEGIN(INITIAL); return 3; }
<STR_STATE>\\n          { parsed_string += '\n'; }
<STR_STATE>\\t          { parsed_string += '\t'; }
<STR_STATE>\\\"         { parsed_string += '\"'; }
<STR_STATE>\\\\         { parsed_string += '\\'; }
<STR_STATE>[^\\\"\n]+   { parsed_string += yytext; }
<STR_STATE>\n           { parsed_string += '\n'; line_number++; }
<STR_STATE><<EOF>>      { std::cerr << "Error: String no cerrado"; return 3; }

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
                            std::cerr << "Lexical Error: Unexpected character '" << yytext[0] 
                                      << "' at line " << line_number << std::endl; 
                        }

%%