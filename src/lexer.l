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

%%

[ \t\r]+                { /* Ignorar espacios, tabulaciones y retornos de carro */ }
"\n"                    { line_number++; }

"//".* { /* Ignorar comentarios de línea */ }

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