#ifndef CODE_GENERATOR_HPP
#define CODE_GENERATOR_HPP

#include <cstddef>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "ast.hpp"
#include "bytecode.hpp"

class CodeGenerator : public Visitor<void> {
public:
    explicit CodeGenerator(BytecodeProgram& program);

    void enterScope();
    void exitScope();

    void visit(NumberLiteral& node) override;
    void visit(StringLiteral& node) override;
    void visit(BoolLiteral& node) override;
    void visit(BinaryExpr& node) override;
    void visit(UnaryExpr& node) override;
    void visit(BlockExpr& node) override;
    void visit(VarRef& node) override;
    void visit(AssignExpr& node) override;
    void visit(LetBinding& node) override;
    void visit(LetExpr& node) override;
    void visit(IfExpr& node) override;
    void visit(WhileExpr& node) override;
    void visit(ForExpr& node) override;
    void visit(FuncCall& node) override;
    void visit(FuncDef& node) override;
    void visit(TypeDef& node) override;
    void visit(ProtocolDef& node) override;
    void visit(ProtocolMethodSig& node) override;
    void visit(NewExpr& node) override;
    void visit(MemberAccess& node) override;
    void visit(MethodCall& node) override;
    void visit(SelfRef& node) override;
    void visit(BaseCall& node) override;
    void visit(IsExpr& node) override;
    void visit(AsExpr& node) override;
    void visit(VectorLiteral& node) override;
    void visit(VectorComprehension& node) override;
    void visit(VectorComprehensionFilter& node) override;
    void visit(VectorIndex& node) override;
    void visit(Program& node) override;

private:
    BytecodeProgram& program_;
    std::size_t constant_counter_ = 0;
    std::vector<std::unordered_set<std::string>> scopes_;
    std::unordered_map<std::string, std::string> type_info_;
};

#endif
