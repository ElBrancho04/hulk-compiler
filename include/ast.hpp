#ifndef AST_HPP
#define AST_HPP

#include <iostream>
#include <vector>
#include <string>
#include <memory>

class NumberLiteral; class StringLiteral; class BoolLiteral;
class BinaryExpr; class UnaryExpr; class BlockExpr;
class VarRef; class AssignExpr; class LetBinding; class LetExpr;
class IfExpr; class WhileExpr; class ForExpr;
class FuncCall; class FuncDef; class TypeDef;
class NewExpr; class MemberAccess; class MethodCall;
class SelfRef; class BaseCall; class IsExpr; class AsExpr;

template <typename T>
class Visitor {
public:
    virtual T visit(NumberLiteral& node) = 0;
    virtual T visit(StringLiteral& node) = 0;
    virtual T visit(BoolLiteral& node) = 0;
    virtual T visit(BinaryExpr& node) = 0;
    virtual T visit(UnaryExpr& node) = 0;
    virtual T visit(BlockExpr& node) = 0;
    virtual T visit(VarRef& node) = 0;
    virtual T visit(AssignExpr& node) = 0;
    virtual T visit(LetBinding& node) = 0;
    virtual T visit(LetExpr& node) = 0;
    virtual T visit(IfExpr& node) = 0;
    virtual T visit(WhileExpr& node) = 0;
    virtual T visit(ForExpr& node) = 0;
    virtual T visit(FuncCall& node) = 0;
    virtual T visit(FuncDef& node) = 0;
    virtual T visit(TypeDef& node) = 0;
    virtual T visit(NewExpr& node) = 0;
    virtual T visit(MemberAccess& node) = 0;
    virtual T visit(MethodCall& node) = 0;
    virtual T visit(SelfRef& node) = 0;
    virtual T visit(BaseCall& node) = 0;
    virtual T visit(IsExpr& node) = 0;
    virtual T visit(AsExpr& node) = 0;
    virtual ~Visitor() = default;
};

class Node {
public:
    int line;
    
    Node(int line) : line(line) {}
    virtual ~Node() = default;

    virtual void accept(Visitor<void>& visitor) = 0;
};

class Expr : public Node {
public:
    Expr(int line) : Node(line) {}
};

class NumberLiteral : public Expr {
public:
    double value;
    NumberLiteral(double value, int line) : Expr(line), value(value) {}

    void accept(Visitor<void>& visitor) override {
        visitor.visit(*this);
    }
};

class StringLiteral : public Expr {
public:
    std::string value;
    StringLiteral(const std::string& value, int line) : Expr(line), value(value) {}

    void accept(Visitor<void>& visitor) override {
        visitor.visit(*this);
    }
};

class BoolLiteral : public Expr {
public:
    bool value;
    BoolLiteral(bool value, int line) : Expr(line), value(value) {}

    void accept(Visitor<void>& visitor) override {
        visitor.visit(*this);
    }
};

struct IfBranch {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Expr> body;

    IfBranch(std::unique_ptr<Expr> cond, std::unique_ptr<Expr> b)
        : condition(std::move(cond)), body(std::move(b)) {}
};

class IfExpr : public Expr {
public:
    std::vector<IfBranch> branches;
    std::unique_ptr<Expr> else_body;

    IfExpr(std::vector<IfBranch> branches, std::unique_ptr<Expr> else_body, int line)
        : Expr(line), branches(std::move(branches)), else_body(std::move(else_body)) {}

    void accept(Visitor<void>& visitor) override { visitor.visit(*this); }
};

class WhileExpr : public Expr {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Expr> body;

    WhileExpr(std::unique_ptr<Expr> condition, std::unique_ptr<Expr> body, int line)
        : Expr(line), condition(std::move(condition)), body(std::move(body)) {}

    void accept(Visitor<void>& visitor) override { visitor.visit(*this); }
};

class ForExpr : public Expr {
public:
    std::string variable_name;
    std::unique_ptr<Expr> iterable;
    std::unique_ptr<Expr> body;

    ForExpr(std::string var_name, std::unique_ptr<Expr> iterable, std::unique_ptr<Expr> body, int line)
        : Expr(line), variable_name(std::move(var_name)), iterable(std::move(iterable)), body(std::move(body)) {}

    void accept(Visitor<void>& visitor) override { visitor.visit(*this); }
};

struct Parameter {
    std::string name;
    std::string type_annotation; // Opcional
    Parameter(std::string n, std::string t = "") : name(std::move(n)), type_annotation(std::move(t)) {}
};

class FuncCall : public Expr {
public:
    std::string name;
    std::vector<std::unique_ptr<Expr>> args;

    FuncCall(std::string name, std::vector<std::unique_ptr<Expr>> args, int line)
        : Expr(line), name(std::move(name)), args(std::move(args)) {}

    void accept(Visitor<void>& visitor) override { visitor.visit(*this); }
};

class FuncDef : public Node {
public:
    std::string name;
    std::vector<Parameter> params;
    std::string return_type;
    std::unique_ptr<Expr> body;

    FuncDef(std::string name, std::vector<Parameter> params, std::string ret_type, std::unique_ptr<Expr> body, int line)
        : Node(line), name(std::move(name)), params(std::move(params)), return_type(std::move(ret_type)), body(std::move(body)) {}

    void accept(Visitor<void>& visitor) override { visitor.visit(*this); }
};

class AttributeDef : public Node {
public:
    std::string name;
    std::string type_annotation;
    std::unique_ptr<Expr> initializer;

    AttributeDef(std::string name, std::string type, std::unique_ptr<Expr> init, int line)
        : Node(line), name(std::move(name)), type_annotation(std::move(type)), initializer(std::move(init)) {}

    void accept(Visitor<void>& visitor) override { /* No requiere visit independiente si se maneja en TypeDef */ }
};

class MethodDef : public Node {
public:
    std::string name;
    std::vector<Parameter> params;
    std::string return_type;
    std::unique_ptr<Expr> body;

    MethodDef(std::string name, std::vector<Parameter> params, std::string ret_type, std::unique_ptr<Expr> body, int line)
        : Node(line), name(std::move(name)), params(std::move(params)), return_type(std::move(ret_type)), body(std::move(body)) {}

    void accept(Visitor<void>& visitor) override { /* Manejado por TypeDef */ }
};

class TypeDef : public Node {
public:
    std::string name;
    std::vector<Parameter> type_params; // Para constructores
    std::string parent_name;
    std::vector<std::unique_ptr<AttributeDef>> attributes;
    std::vector<std::unique_ptr<MethodDef>> methods;

    TypeDef(std::string name, std::vector<Parameter> t_params, std::string p_name, 
            std::vector<std::unique_ptr<AttributeDef>> attrs, std::vector<std::unique_ptr<MethodDef>> meths, int line)
        : Node(line), name(std::move(name)), type_params(std::move(t_params)), 
          parent_name(std::move(p_name)), attributes(std::move(attrs)), methods(std::move(meths)) {}

    void accept(Visitor<void>& visitor) override { visitor.visit(*this); }
};

class NewExpr : public Expr {
public:
    std::string type_name;
    std::vector<std::unique_ptr<Expr>> args;

    NewExpr(std::string type, std::vector<std::unique_ptr<Expr>> args, int line)
        : Expr(line), type_name(std::move(type)), args(std::move(args)) {}

    void accept(Visitor<void>& visitor) override { visitor.visit(*this); }
};

class MemberAccess : public Expr {
public:
    std::unique_ptr<Expr> object;
    std::string member_name;

    MemberAccess(std::unique_ptr<Expr> obj, std::string member, int line)
        : Expr(line), object(std::move(obj)), member_name(std::move(member)) {}

    void accept(Visitor<void>& visitor) override { visitor.visit(*this); }
};

class MethodCall : public Expr {
public:
    std::unique_ptr<Expr> object;
    std::string method_name;
    std::vector<std::unique_ptr<Expr>> args;

    MethodCall(std::unique_ptr<Expr> obj, std::string method, std::vector<std::unique_ptr<Expr>> args, int line)
        : Expr(line), object(std::move(obj)), method_name(std::move(method)), args(std::move(args)) {}

    void accept(Visitor<void>& visitor) override { visitor.visit(*this); }
};

class SelfRef : public Expr {
public:
    SelfRef(int line) : Expr(line) {}
    void accept(Visitor<void>& visitor) override { visitor.visit(*this); }
};

class BaseCall : public Expr {
public:
    std::string method_name;
    std::vector<std::unique_ptr<Expr>> args;

    BaseCall(std::string method, std::vector<std::unique_ptr<Expr>> args, int line)
        : Expr(line), method_name(std::move(method)), args(std::move(args)) {}

    void accept(Visitor<void>& visitor) override { visitor.visit(*this); }
};

class IsExpr : public Expr {
public:
    std::unique_ptr<Expr> expression;
    std::string type_name;

    IsExpr(std::unique_ptr<Expr> expr, std::string type, int line)
        : Expr(line), expression(std::move(expr)), type_name(std::move(type)) {}

    void accept(Visitor<void>& visitor) override { visitor.visit(*this); }
};

class AsExpr : public Expr {
public:
    std::unique_ptr<Expr> expression;
    std::string type_name;

    AsExpr(std::unique_ptr<Expr> expr, std::string type, int line)
        : Expr(line), expression(std::move(expr)), type_name(std::move(type)) {}

    void accept(Visitor<void>& visitor) override { visitor.visit(*this); }
};

#endif