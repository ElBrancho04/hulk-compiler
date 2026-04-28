%{
#include <iostream>
#include <string>
#include "ast.hpp"

// Variable global para el conteo de líneas
int line_number = 1;
%}

/* Opción para no generar la función yywrap (necesaria en versiones antiguas) */
%option noyywrap
/* Evita advertencias de funciones no utilizadas */
%option nounput
%option noinput

/* Definiciones de expresiones regulares */
DIGIT           [0-9]
NUMBER          {DIGIT}+(\.{DIGIT}+)?|\.{DIGIT}+

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

/* Literales Booleanos */
"true"                  { return 1; /* En el futuro será un token TRUE */ }
"false"                 { return 0; /* En el futuro será un token FALSE */ }

/* Flechas y Operadores Compuestos */
"=>"                    { return 200; }
"->"                    { return 201; }
":="                    { return 202; }

/* Comparación */
"=="                    { return 210; }
"!="                    { return 211; }
"<="                    { return 212; }
">="                    { return 213; }
"<"                     { return '<'; }
">"                     { return '>'; }

/* Concatenación */
"@@"                    { return 220; }
"@"                     { return '@'; }

/* Operadores Aritméticos y Booleanos */
"+"                     { return '+'; }
"-"                     { return '-'; }
"*"                     { return '*'; }
"/"                     { return '/'; }
"^"                     { return '^'; }
"&"                     { return '&'; }
"|"                     { return '|'; }
"!"                     { return '!'; }
"="                     { return '='; }

/* Literales Numéricos */
{NUMBER}                { return 2; /* En el futuro será un token NUMBER */ }

/* Literales de String */
\"                      {
                            std::string current_string = "";
                            int c;
                            while ((c = yyinput()) != '\"' && c != EOF) {
                                if (c == '\\') {
                                    c = yyinput();
                                    if (c == 'n') current_string += '\n';
                                    else if (c == 't') current_string += '\t';
                                    else if (c == '\"') current_string += '\"';
                                    else if (c == '\\') current_string += '\\';
                                    else current_string += (char)c;
                                } else if (c == '\n') {
                                    current_string += (char)c;
                                    line_number++;
                                } else {
                                    current_string += (char)c;
                                }
                            }
                            if (c == EOF) {
                                std::cerr << "Error léxico: String no cerrado en línea " << line_number << std::endl;
                            }
                            return 3; /* En el futuro será un token STRING */
                        }

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

%%