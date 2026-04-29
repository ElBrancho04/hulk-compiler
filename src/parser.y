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
%left '*' '/' '%'
%right '^'
%right NEG              /* Unario */

/* Precedencia para llamadas a funciones y acceso a miembros */
%left '(' ')' '[' ']' '.'

%%

program:
    definition_list expression ';' { root = $2; std::cout << "Programa completo parseado." << std::endl; }
    | expression ';' { root = $1; std::cout << "Expresión global procesada." << std::endl; }
    ;

definition_list:
    definition
    | definition_list definition
    ;

definition:
    function_definition
    | type_definition
    | protocol_definition
    ;

/* Definición de Funciones */
function_definition:
    TOK_FUNCTION IDENTIFIER '(' params_list ')' TOK_ARROW expression ';' { std::cout << "Función inline detectada." << std::endl; }
    | TOK_FUNCTION IDENTIFIER '(' params_list ')' expression ';' { std::cout << "Función de bloque detectada." << std::endl; }
    ;

/* Definición de Tipos */
type_definition:
    TOK_TYPE IDENTIFIER opt_type_params opt_inherits '{' type_body '}'
    ;

/* Definición de Protocolos */
protocol_definition:
    TOK_PROTOCOL IDENTIFIER opt_extends '{' protocol_body '}'
    ;

opt_extends:
    /* vacío */
    | TOK_EXTENDS IDENTIFIER
    ;

protocol_body:
    /* vacío */
    | protocol_methods
    ;

protocol_methods:
    protocol_method
    | protocol_methods protocol_method
    ;

/* En un protocolo solo definimos la firma del método */
protocol_method:
    IDENTIFIER '(' params_list ')' ':' IDENTIFIER ';'
    ;

opt_type_params:
    /* vacío */
    | '(' params_list ')'
    ;

opt_inherits:
    /* vacío */
    | TOK_INHERITS IDENTIFIER opt_type_args
    ;

opt_type_args:
    /* vacío */
    | '(' expression_list ')'
    ;

/* Atributos y Métodos */
type_body:
    /* vacío */
    | type_body_elements
    ;

type_body_elements:
    type_member
    | type_body_elements type_member
    ;

type_member:
    attribute_def
    | method_def
    ;

attribute_def:
    IDENTIFIER opt_type_annotation '=' expression ';'
    ;

method_def:
    IDENTIFIER '(' params_list ')' TOK_ARROW expression ';'
    | IDENTIFIER '(' params_list ')' expression ';' 
    ;

/* Anotaciones de tipo ( : TypeName ) */
opt_type_annotation:
    /* vacío */
    | ':' IDENTIFIER
    ;

opt_expression_list:
    /* vacío */
    | expression_list
    ;

expression_list:
    expression
    | expression_list ',' expression
    ;

params_list:
    /* vacío */
    | params_list_not_empty
    ;

params_list_not_empty:
    IDENTIFIER opt_type_annotation
    | params_list_not_empty ',' IDENTIFIER opt_type_annotation
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
    | expression '%' expression { $$ = nullptr; }
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
    
    /* Bloques de expresiones */
    | '{' block_expression_list '}' { $$ = nullptr; }
    
    /* Acceso a miembros y llamadas */
    | expression '.' IDENTIFIER                 { $$ = nullptr; }
    | expression '.' IDENTIFIER '(' opt_type_args ')' { $$ = nullptr; }
    | IDENTIFIER '(' opt_expression_list ')' { $$ = nullptr; }
    
    /* Operadores de Clase */
    | TOK_NEW IDENTIFIER '(' opt_type_args ')'  { $$ = nullptr; }
    | expression TOK_IS IDENTIFIER              { $$ = nullptr; }
    | expression TOK_AS IDENTIFIER              { $$ = nullptr; }
    
    /* Palabras clave especiales */
    | TOK_SELF                                  { $$ = nullptr; }
    | TOK_BASE '(' opt_type_args ')'            { $$ = nullptr; }

    /* Vectores Literales */
    | '[' opt_expression_list ']'               { $$ = nullptr; }

    /* Indexación de vectores */
    | expression '[' expression ']'             { $$ = nullptr; }

    /* Comprensión de vectores (Estructura base) */
    | '[' expression TOK_FOR IDENTIFIER TOK_IN expression ']' { $$ = nullptr; }
    
    /* Comprensión con filtro 'if' */
    | '[' expression TOK_FOR IDENTIFIER TOK_IN expression TOK_IF expression ']' { $$ = nullptr; }
    ;

/* Una lista de expresiones dentro de un bloque, terminadas por ';' */
block_expression_list:
    expression ';'
    | block_expression_list expression ';'
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