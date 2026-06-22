%{
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <cstring>

#include "ast.hpp"

extern int yylex();
extern int line_number;
extern int column_number;
extern char* yytext;
void yyerror(const char *s);

// El nodo raiz
Program* root = nullptr;

struct TypeBodyElements {
    std::vector<class AttributeDef*>* attrs;
    std::vector<class MethodDef*>* methods;
};

struct ProtocolBodyElements {
    std::vector<class ProtocolMethodSig*>* methods;
};

struct InheritsInfo {
    char* name;
    std::vector<class Expr*>* args;
};

struct Definitions {
    std::vector<class TypeDef*>* types;
    std::vector<class ProtocolDef*>* protocols;
    std::vector<class FuncDef*>* functions;
    std::vector<class MacroDef*>* macros;
};

// Helper para mover punteros a unique_ptr
template<typename T>
std::vector<std::unique_ptr<T>> to_unique_vec(std::vector<T*>* src) {
    std::vector<std::unique_ptr<T>> dest;
    if (src) {
        for (auto item : *src) {
            dest.push_back(std::unique_ptr<T>(item));
        }
        delete src;
    }
    return dest;
}
%}

%glr-parser

%union {
    double float_val;
    char* str_val;
    class Node* node_ptr;
    class Expr* expr_ptr;
    class FuncDef* func_ptr;
    class TypeDef* type_ptr;
    class ProtocolDef* protocol_ptr;
    class ProtocolMethodSig* protocol_method_ptr;
    class AttributeDef* attr_ptr;
    class MethodDef* meth_ptr;
    class Parameter* param_ptr;
    std::vector<class Expr*>* expr_list;
    std::vector<class Parameter>* param_list;
    std::vector<class FuncDef*>* func_list;
    std::vector<class TypeDef*>* type_list;
    std::vector<class ProtocolDef*>* protocol_list;
    std::vector<class ProtocolMethodSig*>* protocol_method_list;
    std::vector<class AttributeDef*>* attr_list;
    std::vector<class MethodDef*>* meth_list;
    std::vector<class LetBinding>* let_bindings;
    std::vector<IfBranch>* if_branches;
    struct TypeBodyElements* type_body;
    struct ProtocolBodyElements* protocol_body;
    struct InheritsInfo* inherits_info;
    struct Definitions* definitions;
    class MacroDef* macro_ptr;
    std::vector<class MacroDef*>* macro_list;
    std::vector<MacroParam>* macro_param_list;
    std::vector<MatchArm>* match_arm_list;
}

/* --- Declaracion de Tokens --- */
%token <float_val> NUMBER
%token <str_val> IDENTIFIER STRING
%token TOK_LET TOK_IN TOK_IF TOK_ELSE TOK_ELIF TOK_WHILE TOK_FOR
%token TOK_FUNCTION TOK_TYPE TOK_INHERITS TOK_NEW TOK_IS TOK_AS
%token TOK_PROTOCOL TOK_EXTENDS TOK_SELF TOK_BASE
%token TOK_TRUE TOK_FALSE
%token TOK_DEF TOK_MATCH TOK_CASE TOK_DEFAULT
%token TOK_ARROW TOK_TYPE_ARROW TOK_ASSIGN TOK_EQ TOK_NEQ TOK_LEQ TOK_GEQ TOK_CONCAT TOK_DCONCAT

/* --- Tipos No Terminales --- */
%type <expr_ptr> expression assign_expr let_expr if_expr while_expr for_expr match_expr
%type <expr_ptr> or_expr and_expr not_expr comparison_expr concat_expr add_expr mul_expr pow_expr unary_expr
%type <expr_ptr> postfix_expr primary_expr block_expr opt_vector_filter comp_expr
%type <macro_ptr> macro_definition
%type <macro_param_list> macro_params_list macro_params_not_empty
%type <match_arm_list> match_arms
%type <func_ptr> function_definition
%type <type_ptr> type_definition
%type <protocol_ptr> protocol_definition
%type <str_val> opt_type_annotation opt_return_type opt_extends type_expr func_type_params
%type <expr_list> expression_list opt_expression_list opt_type_args block_expression_list
%type <expr_list> vector_elements opt_vector_elements
%type <param_list> params_list params_list_not_empty opt_type_params
%type <let_bindings> let_bindings
%type <if_branches> if_branches
%type <attr_ptr> attribute_def
%type <meth_ptr> method_def
%type <type_body> type_body_elements
%type <protocol_body> protocol_body_elements
%type <protocol_method_ptr> protocol_method_sig
%type <inherits_info> opt_inherits
%type <definitions> definition_list
%type <expr_ptr> opt_array_init

/* --- Precedencia --- */
%nonassoc TOK_IN
%nonassoc TOK_ELSE
%right TOK_ASSIGN
/* Lambda/inline-body arrows bind looser than every binary operator so that the
   body extends maximally to the right: `f -> a * 2` is `f -> (a*2)`, never
   `(f -> a) * 2`. Without this the lambda (a primary_expr) and the surrounding
   operator create a genuine GLR ambiguity. */
%right TOK_ARROW TOK_TYPE_ARROW
%left '|'
%left '&'
%right '!'
%nonassoc TOK_EQ TOK_NEQ '<' '>' TOK_LEQ TOK_GEQ TOK_IS TOK_AS
%left TOK_CONCAT TOK_DCONCAT
%left '+' '-'
%left '*' '/' '%'
%right '^'
%right NEG

%%

program:
    definition_list expression opt_semicolon
    {
        root = new Program(
            to_unique_vec($1->types),
            to_unique_vec($1->protocols),
            to_unique_vec($1->functions),
            to_unique_vec($1->macros),
            std::unique_ptr<Expr>($2),
            line_number,
            column_number
        );
        delete $1;
    }
    ;

definition_list:
    /* vacio */
    {
        $$ = new Definitions{
            new std::vector<TypeDef*>(),
            new std::vector<ProtocolDef*>(),
            new std::vector<FuncDef*>(),
            new std::vector<MacroDef*>()
        };
    }
    | definition_list function_definition { $1->functions->push_back($2); $$ = $1; }
    | definition_list type_definition { $1->types->push_back($2); $$ = $1; }
    | definition_list protocol_definition { $1->protocols->push_back($2); $$ = $1; }
    | definition_list macro_definition { $1->macros->push_back($2); $$ = $1; }
    ;

function_definition:
    TOK_FUNCTION IDENTIFIER '(' params_list ')' opt_return_type TOK_ARROW expression ';'
    { $$ = new FuncDef($2, *$4, $6 ? $6 : "", std::unique_ptr<Expr>($8), line_number, column_number); delete $4; }
    | TOK_FUNCTION IDENTIFIER '(' params_list ')' opt_return_type block_expr opt_semicolon
    { $$ = new FuncDef($2, *$4, $6 ? $6 : "", std::unique_ptr<Expr>($7), line_number, column_number); delete $4; }
    ;

macro_definition:
    TOK_DEF IDENTIFIER '(' macro_params_list ')' opt_return_type TOK_ARROW expression ';'
    { $$ = new MacroDef($2, std::move(*$4), $6 ? $6 : "", std::unique_ptr<Expr>($8), line_number); delete $4; }
    | TOK_DEF IDENTIFIER '(' macro_params_list ')' opt_return_type TOK_TYPE_ARROW expression ';'
    { $$ = new MacroDef($2, std::move(*$4), $6 ? $6 : "", std::unique_ptr<Expr>($8), line_number); delete $4; }
    | TOK_DEF IDENTIFIER '(' macro_params_list ')' opt_return_type block_expr opt_semicolon
    { $$ = new MacroDef($2, std::move(*$4), $6 ? $6 : "", std::unique_ptr<Expr>($7), line_number); delete $4; }
    ;

macro_params_list:
    /* vacio */ { $$ = new std::vector<MacroParam>(); }
    | macro_params_not_empty { $$ = $1; }
    ;

macro_params_not_empty:
    IDENTIFIER opt_type_annotation
    { $$ = new std::vector<MacroParam>(); $$->emplace_back($1, $2 ? $2 : ""); }
    | '*' IDENTIFIER opt_type_annotation
    { $$ = new std::vector<MacroParam>(); $$->emplace_back($2, $3 ? $3 : "", true); }
    | macro_params_not_empty ',' IDENTIFIER opt_type_annotation
    { $1->emplace_back($3, $4 ? $4 : ""); $$ = $1; }
    | macro_params_not_empty ',' '*' IDENTIFIER opt_type_annotation
    { $1->emplace_back($4, $5 ? $5 : "", true); $$ = $1; }
    ;

type_definition:
    TOK_TYPE IDENTIFIER opt_type_params opt_inherits '{' type_body_elements '}' opt_semicolon
    {
        $$ = new TypeDef(
            $2,
            *$3,
            $4->name ? $4->name : "",
            to_unique_vec($4->args),
            to_unique_vec($6->attrs),
            to_unique_vec($6->methods),
            line_number,
            column_number
        );
        delete $3;
        delete $4;
        delete $6;
    }
    ;

protocol_definition:
    TOK_PROTOCOL IDENTIFIER opt_extends '{' protocol_body_elements '}' opt_semicolon
    {
        $$ = new ProtocolDef(
            $2,
            $3 ? $3 : "",
            to_unique_vec($5->methods),
            line_number,
            column_number
        );
        delete $5;
    }
    ;

comp_expr:
    and_expr { $$ = $1; }
    ;

opt_type_params:
    /* vacio */ { $$ = new std::vector<Parameter>(); }
    | '(' params_list ')' { $$ = $2; }
    ;

opt_return_type:
    /* vacio */ { $$ = nullptr; }
    | ':' type_expr { $$ = $2; }
    ;

opt_inherits:
    /* vacio */ { $$ = new InheritsInfo{nullptr, new std::vector<Expr*>()}; }
    | TOK_INHERITS IDENTIFIER opt_type_args
    {
        $$ = new InheritsInfo{$2, $3};
    }
    ;

opt_extends:
    /* vacio */ { $$ = nullptr; }
    | TOK_EXTENDS IDENTIFIER { $$ = $2; }
    ;

opt_type_args:
    /* vacio */ { $$ = new std::vector<Expr*>(); }
    | '(' expression_list ')' { $$ = $2; }
    ;

params_list:
    /* vacio */ { $$ = new std::vector<Parameter>(); }
    | params_list_not_empty { $$ = $1; }
    ;

params_list_not_empty:
    IDENTIFIER opt_type_annotation { $$ = new std::vector<Parameter>(); $$->emplace_back($1, $2 ? $2 : ""); }
    | params_list_not_empty ',' IDENTIFIER opt_type_annotation { $1->emplace_back($3, $4 ? $4 : ""); $$ = $1; }
    ;

expression:
    assign_expr { $$ = $1; }
    ;

assign_expr:
    IDENTIFIER TOK_ASSIGN assign_expr { $$ = new AssignExpr($1, std::unique_ptr<Expr>($3), line_number, column_number); }
    | postfix_expr '.' IDENTIFIER TOK_ASSIGN assign_expr { $$ = new AssignExpr(std::unique_ptr<Expr>($1), $3, std::unique_ptr<Expr>($5), line_number, column_number); }
    | postfix_expr '[' expression ']' TOK_ASSIGN assign_expr
    {
        $$ = new ArrayAssignExpr(std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3), std::unique_ptr<Expr>($6), line_number, column_number);
    }
    /* Lambdas live at the assignment level (not primary_expr) so the body extends
       maximally to the right and the lambda itself cannot become an operand of a
       binary operator — which would create a genuine `f -> a * b` ambiguity. */
    | '(' ')' opt_return_type TOK_ARROW assign_expr
    { $$ = new LambdaExpr(std::vector<Parameter>(), $3 ? $3 : "", std::unique_ptr<Expr>($5), line_number, column_number); }
    | '(' params_list_not_empty ')' opt_return_type TOK_ARROW assign_expr
    { $$ = new LambdaExpr(*$2, $4 ? $4 : "", std::unique_ptr<Expr>($6), line_number, column_number); delete $2; }
    | TOK_FUNCTION '(' params_list ')' opt_return_type TOK_TYPE_ARROW assign_expr
    { $$ = new LambdaExpr(*$3, $5 ? $5 : "", std::unique_ptr<Expr>($7), line_number, column_number); delete $3; }
    | let_expr { $$ = $1; }
    ;

let_expr:
    TOK_LET let_bindings TOK_IN assign_expr
    {
        $$ = new LetExpr(std::move(*$2), std::unique_ptr<Expr>($4), line_number, column_number);
        delete $2;
    }
    | if_expr { $$ = $1; }
    ;

if_expr:
    TOK_IF '(' expression ')' expression if_branches TOK_ELSE expression
    {
        std::vector<IfBranch> branches;
        branches.emplace_back(std::unique_ptr<Expr>($3), std::unique_ptr<Expr>($5));
        for (auto& br : *$6) {
            branches.emplace_back(std::move(br.condition), std::move(br.body));
        }
        $$ = new IfExpr(std::move(branches), std::unique_ptr<Expr>($8), line_number, column_number);
        delete $6;
    }
    | while_expr { $$ = $1; }
    ;

if_branches:
    /* vacio */ { $$ = new std::vector<IfBranch>(); }
    | if_branches TOK_ELIF '(' expression ')' expression
    {
        $1->emplace_back(std::unique_ptr<Expr>($4), std::unique_ptr<Expr>($6));
        $$ = $1;
    }
    ;

while_expr:
    TOK_WHILE '(' expression ')' expression { $$ = new WhileExpr(std::unique_ptr<Expr>($3), std::unique_ptr<Expr>($5), line_number, column_number); }
    | for_expr { $$ = $1; }
    ;

for_expr:
    TOK_FOR '(' IDENTIFIER TOK_IN expression ')' expression
    { $$ = new ForExpr($3, std::unique_ptr<Expr>($5), std::unique_ptr<Expr>($7), line_number, column_number); }
    | match_expr { $$ = $1; }
    ;

match_expr:
    TOK_MATCH '(' expression ')' '{' match_arms TOK_DEFAULT ':' expression ';' '}'
    {
        $$ = new MatchExpr(std::unique_ptr<Expr>($3), std::move(*$6), std::unique_ptr<Expr>($9), line_number);
        delete $6;
    }
    | or_expr { $$ = $1; }
    ;

match_arms:
    /* vacio */ { $$ = new std::vector<MatchArm>(); }
    | match_arms TOK_CASE IDENTIFIER ':' expression ';'
    {
        $1->emplace_back($3, std::unique_ptr<Expr>($5), line_number);
        $$ = $1;
    }
    ;

or_expr:
    or_expr '|' and_expr { $$ = new BinaryExpr("|", std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3), line_number, column_number); }
    | and_expr { $$ = $1; }
    ;

and_expr:
    and_expr '&' not_expr { $$ = new BinaryExpr("&", std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3), line_number, column_number); }
    | not_expr { $$ = $1; }
    ;

not_expr:
    '!' not_expr { $$ = new UnaryExpr("!", std::unique_ptr<Expr>($2), line_number, column_number); }
    | comparison_expr { $$ = $1; }
    ;

comparison_expr:
    concat_expr { $$ = $1; }
    | concat_expr TOK_EQ concat_expr { $$ = new BinaryExpr("==", std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3), line_number, column_number); }
    | concat_expr TOK_NEQ concat_expr { $$ = new BinaryExpr("!=", std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3), line_number, column_number); }
    | concat_expr '<' concat_expr { $$ = new BinaryExpr("<", std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3), line_number, column_number); }
    | concat_expr '>' concat_expr { $$ = new BinaryExpr(">", std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3), line_number, column_number); }
    | concat_expr TOK_LEQ concat_expr { $$ = new BinaryExpr("<=", std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3), line_number, column_number); }
    | concat_expr TOK_GEQ concat_expr { $$ = new BinaryExpr(">=", std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3), line_number, column_number); }
    | concat_expr TOK_IS IDENTIFIER { $$ = new IsExpr(std::unique_ptr<Expr>($1), $3, line_number, column_number); }
    | concat_expr TOK_AS IDENTIFIER { $$ = new AsExpr(std::unique_ptr<Expr>($1), $3, line_number, column_number); }
    ;

concat_expr:
    concat_expr TOK_CONCAT add_expr { $$ = new BinaryExpr("@", std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3), line_number, column_number); }
    | concat_expr TOK_DCONCAT add_expr { $$ = new BinaryExpr("@@", std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3), line_number, column_number); }
    | add_expr { $$ = $1; }
    ;

add_expr:
    add_expr '+' mul_expr { $$ = new BinaryExpr("+", std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3), line_number, column_number); }
    | add_expr '-' mul_expr { $$ = new BinaryExpr("-", std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3), line_number, column_number); }
    | mul_expr { $$ = $1; }
    ;

mul_expr:
    mul_expr '*' pow_expr { $$ = new BinaryExpr("*", std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3), line_number, column_number); }
    | mul_expr '/' pow_expr { $$ = new BinaryExpr("/", std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3), line_number, column_number); }
    | mul_expr '%' pow_expr { $$ = new BinaryExpr("%", std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3), line_number, column_number); }
    | pow_expr { $$ = $1; }
    ;

pow_expr:
    unary_expr '^' pow_expr { $$ = new BinaryExpr("^", std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3), line_number, column_number); }
    | unary_expr { $$ = $1; }
    ;

unary_expr:
    '-' unary_expr %prec NEG { $$ = new UnaryExpr("-", std::unique_ptr<Expr>($2), line_number, column_number); }
    | postfix_expr { $$ = $1; }
    ;

postfix_expr:
    primary_expr { $$ = $1; }
    | postfix_expr '.' IDENTIFIER { $$ = new MemberAccess(std::unique_ptr<Expr>($1), $3, line_number, column_number); }
    | postfix_expr '.' IDENTIFIER '(' opt_expression_list ')' { $$ = new MethodCall(std::unique_ptr<Expr>($1), $3, to_unique_vec($5), line_number, column_number); }
    | postfix_expr '[' expression ']' { $$ = new VectorIndex(std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3), line_number, column_number); }
    ;

primary_expr:
    NUMBER { $$ = new NumberLiteral($1, line_number, column_number); }
    | STRING { $$ = new StringLiteral($1, line_number, column_number); }
    | TOK_TRUE { $$ = new BoolLiteral(true, line_number, column_number); }
    | TOK_FALSE { $$ = new BoolLiteral(false, line_number, column_number); }
    | IDENTIFIER { $$ = new VarRef($1, line_number, column_number); }
    | IDENTIFIER '(' opt_expression_list ')' { $$ = new FuncCall($1, to_unique_vec($3), line_number, column_number); }
    | IDENTIFIER '(' opt_expression_list ')' block_expr { $$ = new MacroInvoke($1, to_unique_vec($3), std::unique_ptr<Expr>($5), line_number, column_number); }
    | TOK_NEW IDENTIFIER '(' opt_expression_list ')' { $$ = new NewExpr($2, to_unique_vec($4), line_number, column_number); }
    | TOK_NEW IDENTIFIER '[' expression ']' opt_array_init
    {
        $$ = new ArrayNewExpr($2, std::unique_ptr<Expr>($4), std::unique_ptr<Expr>($6), line_number, column_number);
    }
    | TOK_NEW IDENTIFIER '[' ']' '[' expression ']' opt_array_init
    {
        std::string elem_type = std::string($2) + "[]";
        $$ = new ArrayNewExpr(elem_type, std::unique_ptr<Expr>($6), std::unique_ptr<Expr>($8), line_number, column_number);
    }
    | TOK_BASE '(' opt_expression_list ')' { $$ = new BaseCall(to_unique_vec($3), line_number, column_number); }
    | TOK_SELF { $$ = new SelfRef(line_number, column_number); }
    | '(' expression ')' { $$ = $2; }
    | block_expr { $$ = $1; }
    | '[' opt_vector_elements ']' { $$ = new VectorLiteral(to_unique_vec($2), line_number, column_number); }
    | '[' comp_expr '|' IDENTIFIER TOK_IN expression opt_vector_filter ']'
    {
        if ($7) {
            $$ = new VectorComprehensionFilter(
                std::unique_ptr<Expr>($2),
                $4,
                std::unique_ptr<Expr>($6),
                std::unique_ptr<Expr>($7),
                line_number,
                column_number
            );
        } else {
            $$ = new VectorComprehension(
                std::unique_ptr<Expr>($2),
                $4,
                std::unique_ptr<Expr>($6),
                line_number,
                column_number
            );
        }
    }
    ;

block_expr:
    '{' block_expression_list '}' { $$ = new BlockExpr(to_unique_vec($2), line_number, column_number); }
    | '{' comp_expr ',' opt_vector_elements '}'
    {
        auto* vec = new std::vector<Expr*>();
        vec->push_back($2);
        if ($4) {
            for (auto e : *$4) vec->push_back(e);
            delete $4;
        }
        $$ = new VectorLiteral(to_unique_vec(vec), line_number, column_number);
    }
    ;

opt_expression_list:
    /* vacio */ { $$ = new std::vector<Expr*>(); }
    | expression_list { $$ = $1; }
    ;

expression_list:
    expression { $$ = new std::vector<Expr*>(); $$->push_back($1); }
    | expression_list ',' expression { $1->push_back($3); $$ = $1; }
    ;

opt_array_init:
    /* empty */ { $$ = nullptr; }
    | '{' IDENTIFIER TOK_TYPE_ARROW expression '}'
    {
        std::vector<Parameter> params;
        params.emplace_back($2, "");
        $$ = new LambdaExpr(std::move(params), "", std::unique_ptr<Expr>($4), line_number, column_number);
    }
    ;

opt_vector_elements:
    /* vacio */ { $$ = new std::vector<Expr*>(); }
    | vector_elements { $$ = $1; }
    ;

vector_elements:
    comp_expr { $$ = new std::vector<Expr*>(); $$->push_back($1); }
    | vector_elements ',' comp_expr { $1->push_back($3); $$ = $1; }
    ;

type_expr:
    IDENTIFIER
    { $$ = $1; }
    | type_expr '*'
    {
        size_t len = strlen($1) + 12;
        char* buf = (char*)malloc(len);
        sprintf(buf, "Iterable<%s>", $1);
        free($1);
        $$ = buf;
    }
    | type_expr '[' ']'
    {
        size_t len = strlen($1) + 10;
        char* buf = (char*)malloc(len);
        sprintf(buf, "Vector<%s>", $1);
        free($1);
        $$ = buf;
    }
    | '(' func_type_params ')' TOK_TYPE_ARROW type_expr
    {
        // Build: _FuncType(T1,T2)->R
        size_t len = 12 + strlen($2) + strlen($5) + 1; // "_FuncType(" + params + ")->" + ret
        char* buf = (char*)malloc(len);
        sprintf(buf, "_FuncType(%s)->%s", $2, $5);
        free($2);
        free($5);
        $$ = buf;
    }
    ;

func_type_params:
    /* vacio */
    {
        $$ = strdup("");
    }
    | type_expr
    {
        $$ = strdup($1);
    }
    | func_type_params ',' type_expr
    {
        size_t len = strlen($1) + strlen($3) + 2;
        char* buf = (char*)malloc(len);
        sprintf(buf, "%s,%s", $1, $3);
        free($1);
        free($3);
        $$ = buf;
    }
    ;

opt_type_annotation:
    /* vacio */ { $$ = nullptr; }
    | ':' type_expr { $$ = $2; }
    ;

let_bindings:
    IDENTIFIER opt_type_annotation '=' expression
    {
        $$ = new std::vector<LetBinding>();
        $$->emplace_back($1, $2 ? $2 : "", std::unique_ptr<Expr>($4));
    }
    | let_bindings ',' IDENTIFIER opt_type_annotation '=' expression
    {
        $1->emplace_back($3, $4 ? $4 : "", std::unique_ptr<Expr>($6));
        $$ = $1;
    }
    ;

opt_vector_filter:
    /* vacio */ { $$ = nullptr; }
    | TOK_IF expression { $$ = $2; }
    ;

block_expression_list:
    expression ';' { $$ = new std::vector<Expr*>(); $$->push_back($1); }
    | block_expression_list expression ';' { $1->push_back($2); $$ = $1; }
    ;

type_body_elements:
    /* vacio */
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
        $$ = new AttributeDef($1, $2 ? $2 : "", std::unique_ptr<Expr>($4), line_number, column_number);
    }
    ;

method_def:
    IDENTIFIER '(' params_list ')' opt_return_type TOK_ARROW expression ';'
    {
        $$ = new MethodDef($1, *$3, $5 ? $5 : "", std::unique_ptr<Expr>($7), line_number, column_number);
        delete $3;
    }
    | IDENTIFIER '(' params_list ')' opt_return_type block_expr opt_semicolon
    {
        $$ = new MethodDef($1, *$3, $5 ? $5 : "", std::unique_ptr<Expr>($6), line_number, column_number);
        delete $3;
    }
    ;

protocol_body_elements:
    /* vacio */
    {
        $$ = new ProtocolBodyElements{
            new std::vector<ProtocolMethodSig*>()
        };
    }
    | protocol_body_elements protocol_method_sig
    {
        $1->methods->push_back($2);
        $$ = $1;
    }
    ;

protocol_method_sig:
    IDENTIFIER '(' params_list ')' opt_return_type ';'
    {
        $$ = new ProtocolMethodSig($1, *$3, $5 ? $5 : "", line_number, column_number);
        delete $3;
    }
    ;

opt_semicolon:
    /* vacio */
    | ';'
    ;

%%

void yyerror(const char *s) {
    std::cerr << "(" << line_number << "," << column_number << ") SYNTACTIC: " << s
              << " near '" << yytext << "'" << std::endl;
}