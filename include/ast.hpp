#ifndef AST_HPP
#define AST_HPP

#include <iostream>
#include <vector>
#include <string>
#include <memory>

class NumberLiteral;
class StringLiteral;
class BoolLiteral;

template <typename T>
class Visitor {
public:
    virtual T visit(NumberLiteral& node) = 0;
    virtual T visit(StringLiteral& node) = 0;
    virtual T visit(BoolLiteral& node) = 0;
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

#endif