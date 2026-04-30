#ifndef PRINT_VISITOR_HPP
#define PRINT_VISITOR_HPP

#include "ast.hpp"
#include <iostream>
#include <string>

class PrintVisitor : public Visitor<void> {
private:
    int indent_level = 0;

    void printIndent() {
        for (int i = 0; i < indent_level; ++i) std::cout << "  ";
    }

    void nodeHeader(const std::string& name) {
        printIndent();
        std::cout << "[" << name << "]" << std::endl;
    }

public:
    // --- Raíz del Programa ---
    void visit(Program& node) override {
        std::cout << "======= AST VISUALIZER =======" << std::endl;
        nodeHeader("Program");
        indent_level++;
        
        if (!node.types.empty()) {
            printIndent(); std::cout << "Types:" << std::endl;
            indent_level++;
            for (auto& t : node.types) t->accept(*this);
            indent_level--;
        }

        if (!node.protocols.empty()) {
            printIndent(); std::cout << "Protocols:" << std::endl;
            indent_level++;
            for (auto& p : node.protocols) p->accept(*this);
            indent_level--;
        }

        if (!node.functions.empty()) {
            printIndent(); std::cout << "Functions:" << std::endl;
            indent_level++;
            for (auto& f : node.functions) f->accept(*this);
            indent_level--;
        }

        printIndent(); std::cout << "Global Expression:" << std::endl;
        indent_level++;
        if (node.global_expression) node.global_expression->accept(*this);
        indent_level--;

        indent_level--;
        std::cout << "==============================" << std::endl;
    }

    // --- Literales ---
    void visit(NumberLiteral& node) override {
        printIndent();
        std::cout << "Number: " << node.value << " (line " << node.line << ")" << std::endl;
    }

    void visit(StringLiteral& node) override {
        printIndent();
        std::cout << "String: \"" << node.value << "\"" << std::endl;
    }

    void visit(BoolLiteral& node) override {
        printIndent();
        std::cout << "Bool: " << (node.value ? "true" : "false") << std::endl;
    }

    // --- Expresiones ---
    void visit(BinaryExpr& node) override {
        printIndent();
        std::cout << "BinaryOp: " << node.op << std::endl;
        indent_level++;
        node.left->accept(*this);
        node.right->accept(*this);
        indent_level--;
    }

    void visit(UnaryExpr& node) override {
        printIndent();
        std::cout << "UnaryOp: " << node.op << std::endl;
        indent_level++;
        node.operand->accept(*this);
        indent_level--;
    }

    void visit(VarRef& node) override {
        printIndent();
        std::cout << "VarRef: " << node.name << std::endl;
    }

    void visit(BlockExpr& node) override {
        nodeHeader("Block");
        indent_level++;
        for (auto& expr : node.expressions) {
            expr->accept(*this);
        }
        indent_level--;
    }

    void visit(IfExpr& node) override {
        nodeHeader("If-Then-Else");
        indent_level++;
        for (size_t i = 0; i < node.branches.size(); ++i) {
            printIndent(); std::cout << "Branch " << i << " Condition:" << std::endl;
            indent_level++;
            node.branches[i].condition->accept(*this);
            indent_level--;
            printIndent(); std::cout << "Branch " << i << " Body:" << std::endl;
            indent_level++;
            node.branches[i].body->accept(*this);
            indent_level--;
        }
        printIndent(); std::cout << "Else Body:" << std::endl;
        indent_level++;
        node.else_body->accept(*this);
        indent_level--;
        indent_level--;
    }

    void visit(WhileExpr& node) override {
        nodeHeader("While");
        indent_level++;
        printIndent(); std::cout << "Condition:" << std::endl;
        node.condition->accept(*this);
        printIndent(); std::cout << "Body:" << std::endl;
        node.body->accept(*this);
        indent_level--;
    }

    void visit(ForExpr& node) override {
        nodeHeader("For");
        printIndent(); std::cout << "Var: " << node.variable_name << std::endl;
        indent_level++;
        node.iterable->accept(*this);
        node.body->accept(*this);
        indent_level--;
    }

    // --- Funciones y Clases ---
    void visit(FuncDef& node) override {
        printIndent();
        std::cout << "FuncDef: " << node.name << " (Ret: " << (node.return_type.empty() ? "dynamic" : node.return_type) << ")" << std::endl;
        indent_level++;
        for (auto& p : node.params) {
            printIndent(); std::cout << "Param: " << p.name << " : " << p.type_annotation << std::endl;
        }
        node.body->accept(*this);
        indent_level--;
    }

    void visit(TypeDef& node) override {
        printIndent();
        std::cout << "TypeDef: " << node.name << " Inherits: " << (node.parent_name.empty() ? "None" : node.parent_name) << std::endl;
    }

    void visit(ProtocolDef& node) override {
        printIndent();
        std::cout << "ProtocolDef: " << node.name << " Extends: " << (node.parent_name.empty() ? "None" : node.parent_name) << std::endl;
        indent_level++;
        for (auto& m : node.methods) m->accept(*this);
        indent_level--;
    }

    void visit(ProtocolMethodSig& node) override {
        printIndent();
        std::cout << "ProtocolMethod: " << node.name << " (Ret: " << (node.return_type.empty() ? "dynamic" : node.return_type) << ")" << std::endl;
        indent_level++;
        for (auto& p : node.params) {
            printIndent(); std::cout << "Param: " << p.name << " : " << p.type_annotation << std::endl;
        }
        indent_level--;
    }

    void visit(FuncCall& node) override {
        printIndent();
        std::cout << "FuncCall: " << node.name << std::endl;
        indent_level++;
        for (auto& arg : node.args) arg->accept(*this);
        indent_level--;
    }

    // --- Operaciones de Objetos ---
    void visit(NewExpr& node) override {
        printIndent();
        std::cout << "New: " << node.type_name << std::endl;
        indent_level++;
        for (auto& arg : node.args) arg->accept(*this);
        indent_level--;
    }

    void visit(MemberAccess& node) override {
        printIndent();
        std::cout << "MemberAccess: ." << node.member_name << std::endl;
        indent_level++;
        node.object->accept(*this);
        indent_level--;
    }

    void visit(MethodCall& node) override {
        printIndent();
        std::cout << "MethodCall: ." << node.method_name << std::endl;
        indent_level++;
        node.object->accept(*this);
        for (auto& arg : node.args) arg->accept(*this);
        indent_level--;
    }

    void visit(SelfRef& node) override {
        printIndent();
        std::cout << "Self" << std::endl;
    }

    void visit(BaseCall& node) override {
        printIndent();
        std::cout << "BaseCall" << std::endl;
        indent_level++;
        for (auto& arg : node.args) arg->accept(*this);
        indent_level--;
    }

    // --- Vectores ---
    void visit(VectorLiteral& node) override {
        nodeHeader("VectorLiteral");
        indent_level++;
        for (auto& el : node.elements) el->accept(*this);
        indent_level--;
    }

    void visit(VectorIndex& node) override {
        nodeHeader("VectorIndex");
        indent_level++;
        node.vector->accept(*this);
        node.index->accept(*this);
        indent_level--;
    }

    void visit(VectorComprehensionFilter& node) override {
        nodeHeader("VectorComprehensionWithFilter");
        indent_level++;
        printIndent(); std::cout << "Var: " << node.variable_name << std::endl;
        node.generator->accept(*this);
        node.iterable->accept(*this);
        node.filter->accept(*this);
        indent_level--;
    }

    // --- Placeholders para completar la interfaz Visitor ---
    void visit(AssignExpr& node) override {
        nodeHeader("AssignExpr");
        indent_level++;
        printIndent(); std::cout << "Name: " << node.name << std::endl;
        node.value->accept(*this);
        indent_level--;
    }

    void visit(LetBinding& node) override {
        nodeHeader("LetBinding");
        indent_level++;
        printIndent(); std::cout << "Name: " << node.name << " : " << node.type_annotation << std::endl;
        if (node.initializer) node.initializer->accept(*this);
        indent_level--;
    }

    void visit(LetExpr& node) override {
        nodeHeader("LetExpr");
        indent_level++;
        for (auto& b : node.bindings) visit(b);
        if (node.body) node.body->accept(*this);
        indent_level--;
    }

    void visit(IsExpr& node) override {
        nodeHeader("IsExpr");
        indent_level++;
        printIndent(); std::cout << "Type: " << node.type_name << std::endl;
        node.expression->accept(*this);
        indent_level--;
    }

    void visit(AsExpr& node) override {
        nodeHeader("AsExpr");
        indent_level++;
        printIndent(); std::cout << "Type: " << node.type_name << std::endl;
        node.expression->accept(*this);
        indent_level--;
    }

    void visit(VectorComprehension& node) override {
        nodeHeader("VectorComprehension");
        indent_level++;
        printIndent(); std::cout << "Var: " << node.variable_name << std::endl;
        node.generator->accept(*this);
        node.iterable->accept(*this);
        indent_level--;
    }
};

#endif