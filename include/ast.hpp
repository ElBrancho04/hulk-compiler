#ifndef AST_HPP
#define AST_HPP

#include <iostream>
#include <vector>
#include <string>
#include <memory>

class NumberLiteral;
class StringLiteral;
class BoolLiteral;
class BinaryExpr;
class UnaryExpr;
class BlockExpr;
class VarRef;
class AssignExpr;
class LetBinding;
class LetExpr;
class IfExpr;
class WhileExpr;
class ForExpr;

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

#endif