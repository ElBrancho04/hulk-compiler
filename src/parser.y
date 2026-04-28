%{
#include <iostream>
void yyerror(const char *s);
int yylex();
%}
%token NUM
%%
program: /* empty */ ;
%%
void yyerror(const char *s) { std::cerr << s << std::endl; }