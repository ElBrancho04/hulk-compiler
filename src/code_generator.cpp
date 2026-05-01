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

std::size_t CodeGenerator::emit(const Instruction& inst) {
    return program_.addInstruction(inst);
}

std::size_t CodeGenerator::emitJump(OpCode opcode) {
    Instruction inst(opcode);
    inst.offset = 0;
    return emit(inst);
}

void CodeGenerator::patchJump(std::size_t jump_index, std::size_t target_index) {
    if (jump_index >= program_.code.size()) {
        return;
    }

    std::size_t next = jump_index + 1;
    int offset = static_cast<int>(target_index) - static_cast<int>(next);
    program_.code[jump_index].offset = offset;
}

std::size_t CodeGenerator::getConstantIndex(const Value& value, bool dedupe) {
    return program_.addConstant(value, dedupe);
}

std::size_t CodeGenerator::emitLoad(const std::string& name) {
    return emit(Instruction::Load(name));
}

std::size_t CodeGenerator::emitStore(const std::string& name) {
    return emit(Instruction::Store(name));
}

std::size_t CodeGenerator::emitAssign(const std::string& name) {
    return emit(Instruction::Assign(name));
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
