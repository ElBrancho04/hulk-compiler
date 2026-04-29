%{
#include <iostream>
#include <string>
#include <vector>
#include <memory>

#include "ast.hpp" 

extern int yylex();
extern int line_number;
extern char* yytext;
void yyerror(const char *s);

// El nodo raíz
Program* root = nullptr;

struct TypeBodyElements {
    std::vector<class AttributeDef*>* attrs;
    std::vector<class MethodDef*>* methods;
};

// Helper para mover punteros a unique_ptr
template<typename T>
std::vector<std::unique_ptr<T>> to_unique_vec(std::vector<T*>* src) {
    std::vector<std::unique_ptr<T>> dest;
    if (src) {
        for (auto item : *src) {
            dest.push_back(std::unique_ptr<T>(item)); // Transferencia de propiedad
        }
        delete src;
    }
    return dest;
}
%}

%union {
    double float_val;
    char* str_val;
    class Node* node_ptr;
    class Expr* expr_ptr;
    class FuncDef* func_ptr;
    class TypeDef* type_ptr;
    class AttributeDef* attr_ptr;
    class MethodDef* meth_ptr;
    class Parameter* param_ptr;
    std::vector<class Expr*>* expr_list;
    std::vector<class Parameter>* param_list;
    std::vector<class FuncDef*>* func_list;
    std::vector<class TypeDef*>* type_list;
    std::vector<class AttributeDef*>* attr_list;
    std::vector<class MethodDef*>* meth_list;
    std::vector<class LetBinding>* let_bindings;
    std::vector<IfBranch>* if_branches;
    struct TypeBodyElements* type_body;
}

/* --- Declaración de Tokens --- */
%token <float_val> NUMBER
%token <str_val> IDENTIFIER STRING
%token TOK_LET TOK_IN TOK_IF TOK_ELSE TOK_ELIF TOK_WHILE TOK_FOR
%token TOK_FUNCTION TOK_TYPE TOK_INHERITS TOK_NEW TOK_IS TOK_AS
%token TOK_PROTOCOL TOK_EXTENDS TOK_SELF TOK_BASE
%token TOK_TRUE TOK_FALSE
%token TOK_ARROW TOK_TYPE_ARROW TOK_ASSIGN TOK_EQ TOK_NEQ TOK_LEQ TOK_GEQ TOK_CONCAT TOK_DCONCAT

/* --- Tipos No Terminales --- */
%type <expr_ptr> expression
%type <func_ptr> function_definition
%type <type_ptr> type_definition
%type <str_val> opt_type_annotation opt_inherits opt_return_type
%type <expr_list> expression_list opt_expression_list opt_type_args block_expression_list
%type <param_list> params_list params_list_not_empty opt_type_params
%type <func_list> function_list
%type <type_list> type_list
%type <let_bindings> let_bindings
%type <expr_ptr> opt_vector_filter
%type <if_branches> if_branches
%type <expr_ptr> opt_else
%type <attr_ptr> attribute_def
%type <meth_ptr> method_def
%type <type_body> type_body_elements

/* --- Precedencia --- */
%nonassoc TOK_IN
%nonassoc TOK_ELSE
%right TOK_ASSIGN
%right '='
%left '|'
%left '&'
%right '!'
%nonassoc TOK_EQ TOK_NEQ '<' '>' TOK_LEQ TOK_GEQ TOK_IS TOK_AS
%left TOK_CONCAT TOK_DCONCAT
%left '+' '-'
%left '*' '/' '%'
%right '^'
%right NEG
%left '(' ')' '[' ']' '.'

%%

program:
    type_list function_list expression ';' 
    { 
        root = new Program(to_unique_vec($1), to_unique_vec($2), std::unique_ptr<Expr>($3), line_number); 
    }
    | function_list expression ';' 
    { 
        root = new Program({}, to_unique_vec($1), std::unique_ptr<Expr>($2), line_number); 
    }
    | expression ';' 
    { 
        root = new Program({}, {}, std::unique_ptr<Expr>($1), line_number); 
    }
    ;

type_list:
    type_definition { $$ = new std::vector<TypeDef*>(); $$->push_back($1); }
    | type_list type_definition { $1->push_back($2); $$ = $1; }
    ;

function_list:
    function_definition { $$ = new std::vector<FuncDef*>(); $$->push_back($1); }
    | function_list function_definition { $1->push_back($2); $$ = $1; }
    ;

function_definition:
    TOK_FUNCTION IDENTIFIER '(' params_list ')' opt_return_type TOK_ARROW expression ';' 
    { $$ = new FuncDef($2, *$4, $6 ? $6 : "", std::unique_ptr<Expr>($8), line_number); delete $4; }
    | TOK_FUNCTION IDENTIFIER '(' params_list ')' opt_return_type expression ';' 
    { $$ = new FuncDef($2, *$4, $6 ? $6 : "", std::unique_ptr<Expr>($7), line_number); delete $4; }
    ;

type_definition:
    TOK_TYPE IDENTIFIER opt_type_params opt_inherits '{' type_body_elements '}'
    { 
        // $4 es opt_inherits
        $$ = new TypeDef(
            $2,
            *$3,
            $4 ? $4 : "",
            to_unique_vec($6->attrs),
            to_unique_vec($6->methods),
            line_number
        );
        delete $3;
        delete $6;
    }
    ;

opt_type_params:
    /* vacío */ { $$ = new std::vector<Parameter>(); }
    | '(' params_list ')' { $$ = $2; }
    ;

opt_return_type:
    /* vacío */ { $$ = nullptr; }
    | TOK_TYPE_ARROW IDENTIFIER { $$ = $2; }
    ;

opt_inherits:
    /* vacío */ { $$ = nullptr; }
    | TOK_INHERITS IDENTIFIER opt_type_args 
    { 
        // Por ahora ignoramos los argumentos del constructor base ($3) 
        $$ = $2; 
    }
    ;

opt_type_args:
    /* vacío */ { $$ = new std::vector<Expr*>(); }
    | '(' expression_list ')' { $$ = $2; }
    ;

params_list:
    /* vacío */ { $$ = new std::vector<Parameter>(); }
    | params_list_not_empty { $$ = $1; }
    ;

params_list_not_empty:
    IDENTIFIER opt_type_annotation { $$ = new std::vector<Parameter>(); $$->emplace_back($1, $2 ? $2 : ""); }
    | params_list_not_empty ',' IDENTIFIER opt_type_annotation { $1->emplace_back($3, $4 ? $4 : ""); $$ = $1; }
    ;

expression:
    NUMBER          { $$ = new NumberLiteral($1, line_number); }
    | STRING        { $$ = new StringLiteral($1, line_number); }
    | TOK_TRUE      { $$ = new BoolLiteral(true, line_number); }
    | TOK_FALSE     { $$ = new BoolLiteral(false, line_number); }
    | IDENTIFIER    { $$ = new VarRef($1, line_number); }
    | '(' expression ')' { $$ = $2; }
    
    | expression '+' expression { $$ = new BinaryExpr("+", std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3), line_number); }
    | expression '-' expression { $$ = new BinaryExpr("-", std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3), line_number); }
    | expression '*' expression { $$ = new BinaryExpr("*", std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3), line_number); }
    | expression '/' expression { $$ = new BinaryExpr("/", std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3), line_number); }
    | expression '%' expression { $$ = new BinaryExpr("%", std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3), line_number); }
    | expression '^' expression { $$ = new BinaryExpr("^", std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3), line_number); }
    | expression TOK_EQ expression { $$ = new BinaryExpr("==", std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3), line_number); }
    | expression TOK_NEQ expression { $$ = new BinaryExpr("!=", std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3), line_number); }
    | expression '<' expression { $$ = new BinaryExpr("<", std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3), line_number); }
    | expression '>' expression { $$ = new BinaryExpr(">", std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3), line_number); }
    | expression TOK_LEQ expression { $$ = new BinaryExpr("<=", std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3), line_number); }
    | expression TOK_GEQ expression { $$ = new BinaryExpr(">=", std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3), line_number); }
    | '-' expression %prec NEG  { $$ = new UnaryExpr("-", std::unique_ptr<Expr>($2), line_number); }
    | IDENTIFIER '=' expression { $$ = new AssignExpr($1, std::unique_ptr<Expr>($3), line_number); }
    | IDENTIFIER TOK_ASSIGN expression { $$ = new AssignExpr($1, std::unique_ptr<Expr>($3), line_number); }

    | TOK_IF '(' expression ')' expression if_branches opt_else
    {
        std::vector<IfBranch> branches;
        branches.emplace_back(std::unique_ptr<Expr>($3), std::unique_ptr<Expr>($5));
        for (auto& br : *$6) {
            branches.emplace_back(std::move(br.condition), std::move(br.body));
        }
        std::unique_ptr<Expr> else_body($7);
        $$ = new IfExpr(std::move(branches), std::move(else_body), line_number);
        delete $6;
    }
    | TOK_WHILE '(' expression ')' expression { $$ = new WhileExpr(std::unique_ptr<Expr>($3), std::unique_ptr<Expr>($5), line_number); }
    | TOK_FOR '(' IDENTIFIER TOK_IN expression ')' expression { $$ = new ForExpr($3, std::unique_ptr<Expr>($5), std::unique_ptr<Expr>($7), line_number); }
    
    | TOK_NEW IDENTIFIER opt_type_args { $$ = new NewExpr($2, to_unique_vec($3), line_number); }
    | expression '.' IDENTIFIER { $$ = new MemberAccess(std::unique_ptr<Expr>($1), $3, line_number); }
    | expression '.' IDENTIFIER '(' opt_expression_list ')' { $$ = new MethodCall(std::unique_ptr<Expr>($1), $3, to_unique_vec($5), line_number); }
    | IDENTIFIER '(' opt_expression_list ')' { $$ = new FuncCall($1, to_unique_vec($3), line_number); }
    | TOK_BASE '.' IDENTIFIER '(' opt_expression_list ')' { $$ = new BaseCall($3, to_unique_vec($5), line_number); }
    | TOK_SELF { $$ = new SelfRef(line_number); }
    | expression TOK_IS IDENTIFIER { $$ = new IsExpr(std::unique_ptr<Expr>($1), $3, line_number); }
    | expression TOK_AS IDENTIFIER { $$ = new AsExpr(std::unique_ptr<Expr>($1), $3, line_number); }

    | '{' block_expression_list '}' { $$ = new BlockExpr(to_unique_vec($2), line_number); }

    | '[' opt_expression_list ']' { $$ = new VectorLiteral(to_unique_vec($2), line_number); }
    | expression '[' expression ']' { $$ = new VectorIndex(std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3), line_number); }
    | '[' expression '|' IDENTIFIER TOK_IN expression opt_vector_filter ']'
    {
        if ($7) {
            $$ = new VectorComprehensionFilter(
                std::unique_ptr<Expr>($2),
                $4,
                std::unique_ptr<Expr>($6),
                std::unique_ptr<Expr>($7),
                line_number
            );
        } else {
            $$ = new VectorComprehension(
                std::unique_ptr<Expr>($2),
                $4,
                std::unique_ptr<Expr>($6),
                line_number
            );
        }
    }
    | TOK_LET let_bindings TOK_IN expression
    {
        $$ = new LetExpr(std::move(*$2), std::unique_ptr<Expr>($4), line_number);
        delete $2;
    }
    ;

if_branches:
    /* vacío */ { $$ = new std::vector<IfBranch>(); }
    | if_branches TOK_ELIF '(' expression ')' expression
    {
        $1->emplace_back(std::unique_ptr<Expr>($4), std::unique_ptr<Expr>($6));
        $$ = $1;
    }
    ;

opt_else:
    /* vacío */ { $$ = nullptr; }
    | TOK_ELSE expression { $$ = $2; }
    ;

opt_vector_filter:
    /* vacío */ { $$ = nullptr; }
    | TOK_IF expression { $$ = $2; }
    ;

block_expression_list:
    expression ';' { $$ = new std::vector<Expr*>(); $$->push_back($1); }
    | block_expression_list expression ';' { $1->push_back($2); $$ = $1; }
    ;

opt_expression_list:
    /* vacío */ { $$ = new std::vector<Expr*>(); }
    | expression_list { $$ = $1; }
    ;

expression_list:
    expression { $$ = new std::vector<Expr*>(); $$->push_back($1); }
    | expression_list ',' expression { $1->push_back($3); $$ = $1; }
    ;

opt_type_annotation:
    /* vacío */ { $$ = nullptr; }
    | ':' IDENTIFIER { $$ = $2; }
    ;

let_bindings:
    IDENTIFIER '=' expression
    {
        $$ = new std::vector<LetBinding>();
        $$->emplace_back($1, std::unique_ptr<Expr>($3));
    }
    | let_bindings ',' IDENTIFIER '=' expression
    {
        $1->emplace_back($3, std::unique_ptr<Expr>($5));
        $$ = $1;
    }
    ;

type_body_elements:
    /* vacío */
    {
        $$ = new TypeBodyElements{
            new std::vector<AttributeDef*>(),
            new std::vector<MethodDef*>()
        };
    }
    | type_body_elements attribute_def
    {
        $1->attrs->push_back($2);
        $$ = $1;
    }
    | type_body_elements method_def
    {
        $1->methods->push_back($2);
        $$ = $1;
    }
    ;

attribute_def:
    IDENTIFIER opt_type_annotation '=' expression ';'
    {
        $$ = new AttributeDef($1, $2 ? $2 : "", std::unique_ptr<Expr>($4), line_number);
    }
    ;

method_def:
    IDENTIFIER '(' params_list ')' opt_return_type TOK_ARROW expression ';'
    {
        $$ = new MethodDef($1, *$3, $5 ? $5 : "", std::unique_ptr<Expr>($7), line_number);
        delete $3;
    }
    ;

%%

void yyerror(const char *s) {
    std::cerr << "Syntax Error: " << s << " at line " << line_number << " near '" << yytext << "'" << std::endl;
}