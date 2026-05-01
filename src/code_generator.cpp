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

std::string CodeGenerator::makeTemp(const std::string& prefix) {
    return prefix + std::to_string(temp_counter_++);
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
    emit(Instruction(OpCode::POP_SCOPE));
}
void CodeGenerator::visit(IfExpr& node) {
    std::vector<std::size_t> end_jumps;

    for (auto& branch : node.branches) {
        if (branch.condition) {
            branch.condition->accept(*this);
        }

        std::size_t false_jump = emitJump(OpCode::JUMP_IF_FALSE);

        if (branch.body) {
            branch.body->accept(*this);
        }

        std::size_t end_jump = emitJump(OpCode::JUMP);
        end_jumps.push_back(end_jump);

        patchJump(false_jump, program_.code.size());
    }

    if (node.else_body) {
        node.else_body->accept(*this);
    }

    std::size_t end_target = program_.code.size();
    for (std::size_t jump : end_jumps) {
        patchJump(jump, end_target);
    }
}

void CodeGenerator::visit(WhileExpr& node) {
    std::size_t loop_start = program_.code.size();

    if (node.condition) {
        node.condition->accept(*this);
    }

    std::size_t exit_jump = emitJump(OpCode::JUMP_IF_FALSE);

    if (node.body) {
        node.body->accept(*this);
    }

    std::size_t back_jump = emitJump(OpCode::JUMP);
    patchJump(back_jump, loop_start);
    patchJump(exit_jump, program_.code.size());
}

void CodeGenerator::visit(ForExpr& node) {
    enterScope();

    if (node.iterable) {
        node.iterable->accept(*this);
    }

    std::size_t loop_start = program_.code.size();
    emit(Instruction(OpCode::ITER_NEXT));
    std::size_t exit_jump = emitJump(OpCode::JUMP_IF_FALSE);

    emit(Instruction(OpCode::ITER_CURRENT));
    emitStore(node.variable_name);

    if (node.body) {
        node.body->accept(*this);
    }

    std::size_t back_jump = emitJump(OpCode::JUMP);
    patchJump(back_jump, loop_start);
    patchJump(exit_jump, program_.code.size());

    exitScope();
}
void CodeGenerator::visit(FuncCall& node) {
    for (auto& arg : node.args) {
        if (arg) {
            arg->accept(*this);
        }
    }

    emit(Instruction::Call(node.name, static_cast<int>(node.args.size())));
}

void CodeGenerator::visit(FuncDef& node) {
    std::string previous_method = current_method_;
    current_method_.clear();
    std::size_t start_index = program_.code.size();
    program_.addFunctionSymbol(node.name, start_index);
    emit(Instruction::Label(static_cast<int>(start_index)));

    enterScope();
    for (const auto& param : node.params) {
        emitStore(param.name);
    }

    if (node.body) {
        node.body->accept(*this);
    }

    emit(Instruction(OpCode::RETURN));
    exitScope();
    current_method_ = previous_method;
}
void CodeGenerator::visit(TypeDef& node) {
    for (auto& method : node.methods) {
        if (!method) {
            continue;
        }

        std::string previous_method = current_method_;
        current_method_ = method->name;

        std::string symbol = node.name + "." + method->name;
        std::size_t start_index = program_.code.size();
        program_.addFunctionSymbol(symbol, start_index);
        emit(Instruction::Label(static_cast<int>(start_index)));

        enterScope();
        for (const auto& param : method->params) {
            emitStore(param.name);
        }

        if (method->body) {
            method->body->accept(*this);
        }

        emit(Instruction(OpCode::RETURN));
        exitScope();

        current_method_ = previous_method;
    }
}
void CodeGenerator::visit(ProtocolDef&) {}
void CodeGenerator::visit(ProtocolMethodSig&) {}

void CodeGenerator::visit(NewExpr& node) {
    for (auto& arg : node.args) {
        if (arg) {
            arg->accept(*this);
        }
    }

    emit(Instruction::New(node.type_name, static_cast<int>(node.args.size())));
}

void CodeGenerator::visit(MemberAccess& node) {
    if (node.object) {
        node.object->accept(*this);
    }

    emit(Instruction::GetAttr(node.member_name));
}

void CodeGenerator::visit(MethodCall& node) {
    if (node.object) {
        node.object->accept(*this);
    }

    for (auto& arg : node.args) {
        if (arg) {
            arg->accept(*this);
        }
    }

    emit(Instruction::MethodCall(node.method_name, static_cast<int>(node.args.size())));
}

void CodeGenerator::visit(SelfRef&) {
    emit(Instruction(OpCode::SELF));
}

void CodeGenerator::visit(BaseCall& node) {
    for (auto& arg : node.args) {
        if (arg) {
            arg->accept(*this);
        }
    }

    std::string target = current_method_.empty() ? "base" : current_method_;
    emit(Instruction::BaseCall(target, static_cast<int>(node.args.size())));
}
void CodeGenerator::visit(IsExpr& node) {
    if (node.expression) {
        node.expression->accept(*this);
    }
    emit(Instruction::Is(node.type_name));
}

void CodeGenerator::visit(AsExpr& node) {
    if (node.expression) {
        node.expression->accept(*this);
    }
    emit(Instruction::As(node.type_name));
}

void CodeGenerator::visit(VectorLiteral& node) {
    if (!node.elements.empty()) {
        for (std::size_t i = node.elements.size(); i > 0; --i) {
            auto& element = node.elements[i - 1];
            if (element) {
                element->accept(*this);
            }
        }
    }

    emit(Instruction::NewVector(static_cast<int>(node.elements.size())));
}

void CodeGenerator::visit(VectorComprehension& node) {
    enterScope();

    std::string iter_name = makeTemp("_iter");
    std::string vec_name = makeTemp("_vec");

    if (node.iterable) {
        node.iterable->accept(*this);
    }
    emitStore(iter_name);

    emit(Instruction::VectorInit());
    emitStore(vec_name);

    std::size_t loop_start = program_.code.size();
    emitLoad(iter_name);
    emit(Instruction(OpCode::ITER_NEXT));
    std::size_t exit_jump = emitJump(OpCode::JUMP_IF_FALSE);

    emitLoad(iter_name);
    emit(Instruction(OpCode::ITER_CURRENT));
    emitStore(node.variable_name);

    emitLoad(vec_name);
    if (node.generator) {
        node.generator->accept(*this);
    }
    emit(Instruction::VectorPush());
    emitStore(vec_name);

    std::size_t back_jump = emitJump(OpCode::JUMP);
    patchJump(back_jump, loop_start);
    patchJump(exit_jump, program_.code.size());

    emitLoad(vec_name);
    exitScope();
}

void CodeGenerator::visit(VectorComprehensionFilter& node) {
    enterScope();

    std::string iter_name = makeTemp("_iter");
    std::string vec_name = makeTemp("_vec");

    if (node.iterable) {
        node.iterable->accept(*this);
    }
    emitStore(iter_name);

    emit(Instruction::VectorInit());
    emitStore(vec_name);

    std::size_t loop_start = program_.code.size();
    emitLoad(iter_name);
    emit(Instruction(OpCode::ITER_NEXT));
    std::size_t exit_jump = emitJump(OpCode::JUMP_IF_FALSE);

    emitLoad(iter_name);
    emit(Instruction(OpCode::ITER_CURRENT));
    emitStore(node.variable_name);

    if (node.filter) {
        node.filter->accept(*this);
    }
    std::size_t skip_jump = emitJump(OpCode::JUMP_IF_FALSE);

    emitLoad(vec_name);
    if (node.generator) {
        node.generator->accept(*this);
    }
    emit(Instruction::VectorPush());
    emitStore(vec_name);

    patchJump(skip_jump, program_.code.size());

    std::size_t back_jump = emitJump(OpCode::JUMP);
    patchJump(back_jump, loop_start);
    patchJump(exit_jump, program_.code.size());

    emitLoad(vec_name);
    exitScope();
}

void CodeGenerator::visit(VectorIndex& node) {
    if (node.vector) {
        node.vector->accept(*this);
    }
    if (node.index) {
        node.index->accept(*this);
    }
    emit(Instruction(OpCode::VECTOR_INDEX));
}
void CodeGenerator::visit(Program& node) {
    for (auto& type_def : node.types) {
        if (type_def) {
            type_def->accept(*this);
        }
    }

    for (auto& func : node.functions) {
        if (func) {
            func->accept(*this);
        }
    }

    if (node.global_expression) {
        node.global_expression->accept(*this);
    }

    emit(Instruction(OpCode::HALT));
}
