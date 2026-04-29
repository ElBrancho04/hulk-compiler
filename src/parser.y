%{
#include <iostream>
#include <string>
#include "ast.hpp"

// Prototipos y variables externas
extern int yylex();
extern int line_number;
extern char* yytext;
void yyerror(const char *s);

// El nodo raíz que contendrá todo el programa
Node* root = nullptr;
%}

/* Unión para los valores semánticos de los tokens */
%union {
    double float_val;
    char* str_val;
    class Node* node_ptr;
}

/* --- Declaración de Tokens --- */

/* Literales y nombres */
%token <float_val> NUMBER
%token <str_val> IDENTIFIER STRING

/* Palabras Reservadas */
%token TOK_LET TOK_IN TOK_IF TOK_ELSE TOK_ELIF TOK_WHILE TOK_FOR
%token TOK_FUNCTION TOK_TYPE TOK_INHERITS TOK_NEW TOK_IS TOK_AS
%token TOK_PROTOCOL TOK_EXTENDS TOK_SELF TOK_BASE
%token TOK_TRUE TOK_FALSE

/* Operadores Compuestos */
%token TOK_ARROW       /* => */
%token TOK_TYPE_ARROW  /* -> */
%token TOK_ASSIGN      /* := */
%token TOK_EQ          /* == */
%token TOK_NEQ         /* != */
%token TOK_LEQ         /* <= */
%token TOK_GEQ         /* >= */
%token TOK_CONCAT      /* @  */
%token TOK_DCONCAT     /* @@ */

%type <node_ptr> expression

/* --- Precedencia y Asociatividad --- */
/* De menor a mayor precedencia */
%nonassoc TOK_IN        /* Para resolver ambigüedades en LET */
%nonassoc TOK_ELSE      /* Para resolver el 'Dangling Else' */

%right TOK_ASSIGN       /* := */
%left '|'
%left '&'
%right '!'
%nonassoc TOK_EQ TOK_NEQ '<' '>' TOK_LEQ TOK_GEQ TOK_IS TOK_AS
%left TOK_CONCAT TOK_DCONCAT
%left '+' '-'
%left '*' '/'
%right '^'
%right NEG              /* Unario */

/* Precedencia para llamadas a funciones y acceso a miembros */
%left '(' ')' '[' ']' '.'

%%

program:
    instruction_list
    ;

instruction_list:
    instruction
    | instruction_list instruction
    ;

instruction:
    expression ';' { std::cout << "Sentencia procesada con éxito." << std::endl; }
    ;

expression:
    /* Literales */
    NUMBER                  { $$ = nullptr; }
    | STRING                { $$ = nullptr; }
    | TOK_TRUE              { $$ = nullptr; }
    | TOK_FALSE             { $$ = nullptr; }
    | IDENTIFIER            { $$ = nullptr; }
    
    /* Paréntesis */
    | '(' expression ')'    { $$ = $2; }

    /* Aritmética */
    | expression '+' expression { $$ = nullptr; }
    | expression '-' expression { $$ = nullptr; }
    | expression '*' expression { $$ = nullptr; }
    | expression '/' expression { $$ = nullptr; }
    | expression '^' expression { $$ = nullptr; }
    | '-' expression %prec NEG  { $$ = nullptr; }

    /* Comparaciones */
    | expression TOK_EQ expression  { $$ = nullptr; }
    | expression TOK_NEQ expression { $$ = nullptr; }
    | expression '<' expression     { $$ = nullptr; }
    | expression '>' expression     { $$ = nullptr; }
    | expression TOK_LEQ expression { $$ = nullptr; }
    | expression TOK_GEQ expression { $$ = nullptr; }

    /* Lógica */
    | expression '|' expression     { $$ = nullptr; }
    | expression '&' expression     { $$ = nullptr; }
    | '!' expression                { $$ = nullptr; }

    /* Concatenación */
    | expression TOK_CONCAT expression  { $$ = nullptr; }
    | expression TOK_DCONCAT expression { $$ = nullptr; }

    /* Asignación */
    | IDENTIFIER TOK_ASSIGN expression  { $$ = nullptr; }

    /* Let */
    | TOK_LET binding_list TOK_IN expression { $$ = nullptr; }

    /* If-Else */
    | TOK_IF '(' expression ')' expression TOK_ELSE expression { $$ = nullptr; }

    /* Bucles */
    | TOK_WHILE '(' expression ')' expression { $$ = nullptr; }
    | TOK_FOR '(' IDENTIFIER TOK_IN expression ')' expression { $$ = nullptr; }
    ;

binding_list:
    binding                 { /* No tiene tipo asignado aún */ }
    | binding_list ',' binding
    ;

binding:
    IDENTIFIER '=' expression
    ;

%%

void yyerror(const char *s) {
    std::cerr << "Syntax Error: " << s << " at line " << line_number << " near '" << yytext << "'" << std::endl;
}