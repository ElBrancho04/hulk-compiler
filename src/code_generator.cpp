#include "code_generator.hpp"

CodeGenerator::CodeGenerator(BytecodeProgram& program)
    : program_(program) {
    enterScope();
}

void CodeGenerator::enterScope() {
    scopes_.emplace_back();
}

void CodeGenerator::exitScope() {
    if (!scopes_.empty()) {
        scopes_.pop_back();
    }
}

void CodeGenerator::visit(NumberLiteral&) {}
void CodeGenerator::visit(StringLiteral&) {}
void CodeGenerator::visit(BoolLiteral&) {}
void CodeGenerator::visit(BinaryExpr&) {}
void CodeGenerator::visit(UnaryExpr&) {}
void CodeGenerator::visit(BlockExpr&) {}
void CodeGenerator::visit(VarRef&) {}
void CodeGenerator::visit(AssignExpr&) {}
void CodeGenerator::visit(LetBinding&) {}
void CodeGenerator::visit(LetExpr&) {}
void CodeGenerator::visit(IfExpr&) {}
void CodeGenerator::visit(WhileExpr&) {}
void CodeGenerator::visit(ForExpr&) {}
void CodeGenerator::visit(FuncCall&) {}
void CodeGenerator::visit(FuncDef&) {}
void CodeGenerator::visit(TypeDef&) {}
void CodeGenerator::visit(ProtocolDef&) {}
void CodeGenerator::visit(ProtocolMethodSig&) {}
void CodeGenerator::visit(NewExpr&) {}
void CodeGenerator::visit(MemberAccess&) {}
void CodeGenerator::visit(MethodCall&) {}
void CodeGenerator::visit(SelfRef&) {}
void CodeGenerator::visit(BaseCall&) {}
void CodeGenerator::visit(IsExpr&) {}
void CodeGenerator::visit(AsExpr&) {}
void CodeGenerator::visit(VectorLiteral&) {}
void CodeGenerator::visit(VectorComprehension&) {}
void CodeGenerator::visit(VectorComprehensionFilter&) {}
void CodeGenerator::visit(VectorIndex&) {}
void CodeGenerator::visit(Program&) {}
