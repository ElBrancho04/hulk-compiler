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

void CodeGenerator::visit(NumberLiteral& node) {
    std::size_t index = getConstantIndex(Value::Number(node.value));
    emit(Instruction::PushConst(static_cast<int>(index)));
}

void CodeGenerator::visit(StringLiteral& node) {
    std::size_t index = getConstantIndex(Value::String(node.value));
    emit(Instruction::PushConst(static_cast<int>(index)));
}

void CodeGenerator::visit(BoolLiteral& node) {
    std::size_t index = getConstantIndex(Value::Boolean(node.value));
    emit(Instruction::PushConst(static_cast<int>(index)));
}

void CodeGenerator::visit(BinaryExpr& node) {
    if (node.left) {
        node.left->accept(*this);
    }
    if (node.right) {
        node.right->accept(*this);
    }

    if (node.op == "+") {
        emit(Instruction(OpCode::ADD));
    } else if (node.op == "-") {
        emit(Instruction(OpCode::SUB));
    } else if (node.op == "*") {
        emit(Instruction(OpCode::MUL));
    } else if (node.op == "/") {
        emit(Instruction(OpCode::DIV));
    } else if (node.op == "^") {
        emit(Instruction(OpCode::POW));
    } else if (node.op == "&") {
        emit(Instruction(OpCode::AND));
    } else if (node.op == "|") {
        emit(Instruction(OpCode::OR));
    } else if (node.op == "@") {
        emit(Instruction(OpCode::CONCAT));
    } else if (node.op == "@@") {
        emit(Instruction(OpCode::CONCAT_SPACE));
    } else if (node.op == "==") {
        emit(Instruction(OpCode::CMP_EQ));
    } else if (node.op == "!=") {
        emit(Instruction(OpCode::CMP_NEQ));
    } else if (node.op == "<") {
        emit(Instruction(OpCode::CMP_LT));
    } else if (node.op == ">") {
        emit(Instruction(OpCode::CMP_GT));
    } else if (node.op == "<=") {
        emit(Instruction(OpCode::CMP_LE));
    } else if (node.op == ">=") {
        emit(Instruction(OpCode::CMP_GE));
    }
}

void CodeGenerator::visit(UnaryExpr& node) {
    if (node.operand) {
        node.operand->accept(*this);
    }

    if (node.op == "-") {
        emit(Instruction(OpCode::NEG));
    } else if (node.op == "!") {
        emit(Instruction(OpCode::NOT));
    }
}

void CodeGenerator::visit(BlockExpr& node) {
    for (std::size_t i = 0; i < node.expressions.size(); ++i) {
        if (node.expressions[i]) {
            node.expressions[i]->accept(*this);
        }

        if (i + 1 < node.expressions.size()) {
            emit(Instruction(OpCode::POP));
        }
    }
}

void CodeGenerator::visit(VarRef& node) {
    emitLoad(node.name);
}

void CodeGenerator::visit(AssignExpr& node) {
    if (node.value) {
        node.value->accept(*this);
    }
    emitAssign(node.name);
}
void CodeGenerator::visit(LetBinding&) {}
void CodeGenerator::visit(LetExpr& node) {
    enterScope();

    for (auto& binding : node.bindings) {
        if (binding.initializer) {
            binding.initializer->accept(*this);
        }
        emitStore(binding.name);
    }

    if (node.body) {
        node.body->accept(*this);
    }

    exitScope();
}
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
