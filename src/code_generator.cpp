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
    } else if (node.op == "%") {
        emit(Instruction(OpCode::MOD));
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
    emit(Instruction(OpCode::BEGIN_SCOPE));

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

    std::string iter_var = makeTemp("_iter");
    std::string result_var = makeTemp("_result");

    // result starts as null (returned if zero iterations)
    std::size_t null_idx = getConstantIndex(Value::Null());
    emit(Instruction::PushConst(static_cast<int>(null_idx)));
    emitStore(result_var);

    if (node.iterable) {
        node.iterable->accept(*this);
    }
    emitStore(iter_var);

    std::size_t loop_start = program_.code.size();
    emitLoad(iter_var);
    emit(Instruction(OpCode::ITER_NEXT));
    std::size_t exit_jump = emitJump(OpCode::JUMP_IF_FALSE);

    emitLoad(iter_var);
    emit(Instruction(OpCode::ITER_CURRENT));
    emitStore(node.variable_name);

    if (node.body) {
        node.body->accept(*this);
    }
    emitStore(result_var);  // save body result; net stack-neutral

    std::size_t back_jump = emitJump(OpCode::JUMP);
    patchJump(back_jump, loop_start);
    patchJump(exit_jump, program_.code.size());

    emitLoad(result_var);  // for-expression value = last body result (or null)
    exitScope();
}
void CodeGenerator::visit(FuncCall& node) {
    if (node.is_functor) {
        // Llamada a functor: f(x) → f.invoke(x)
        // Stack esperado por METHOD_CALL: [objeto, arg1, ..., argN]
        emitLoad(node.name);
        for (auto& arg : node.args) {
            if (arg) { arg->accept(*this); }
        }
        emit(Instruction::MethodCall("invoke", static_cast<int>(node.args.size())));
        return;
    }

    // Llamada a función global normal
    for (auto& arg : node.args) {
        if (arg) { arg->accept(*this); }
    }
    emit(Instruction::Call(node.name, static_cast<int>(node.args.size())));
}

void CodeGenerator::visit(FuncDef& node) {
    std::string previous_method = current_method_;
    current_method_.clear();

    // Emit a JUMP placeholder so the function body is skipped during linear
    // execution. CALL jumps directly to the body via function_table, bypassing
    // this JUMP.
    std::size_t skip_jump = emitJump(OpCode::JUMP);

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

    // Patch the skip JUMP to land just past the function body.
    patchJump(skip_jump, program_.code.size());

    current_method_ = previous_method;
}
void CodeGenerator::visit(TypeDef& node) {
    std::string previous_type = current_type_name_;
    current_type_name_ = node.name;

    // Generate __init__ method that initializes attributes and calls parent constructor
    {
        std::string init_symbol = node.name + ".__init__";

        // Emit a JUMP placeholder so the __init__ body is skipped during linear
        // execution. METHOD_CALL jumps directly to the body via function_table.
        std::size_t skip_jump = emitJump(OpCode::JUMP);

        std::size_t start_index = program_.code.size();
        program_.addFunctionSymbol(init_symbol, start_index);
        emit(Instruction::Label(static_cast<int>(start_index)));

        enterScope();
        // Store constructor params in local scope
        for (const auto& param : node.type_params) {
            emitStore(param.name);
        }

        // Call parent's __init__ if there is inheritance
        if (!node.parent_name.empty()) {
            emit(Instruction(OpCode::SELF));
            for (auto& parent_arg : node.parent_args) {
                if (parent_arg) {
                    parent_arg->accept(*this);
                }
            }
            emit(Instruction::MethodCall("__init__", static_cast<int>(node.parent_args.size())));
            emit(Instruction(OpCode::POP));
        }

        // Assign constructor args to attributes via SET_ATTR
        for (const auto& param : node.type_params) {
            emit(Instruction(OpCode::SELF));
            emitLoad(param.name);
            emit(Instruction::SetAttr(param.name));
            emit(Instruction(OpCode::POP));
        }

        // Evaluate attribute initializers for attributes without matching constructor params
        for (const auto& attr : node.attributes) {
            if (attr && attr->initializer) {
                bool is_constructor_param = false;
                for (const auto& param : node.type_params) {
                    if (param.name == attr->name) {
                        is_constructor_param = true;
                        break;
                    }
                }
                if (!is_constructor_param) {
                    emit(Instruction(OpCode::SELF));
                    attr->initializer->accept(*this);
                    emit(Instruction::SetAttr(attr->name));
                    emit(Instruction(OpCode::POP));
                }
            }
        }

        // Return self
        emit(Instruction(OpCode::SELF));
        emit(Instruction(OpCode::RETURN));
        exitScope();

        // Patch the skip JUMP to land just past __init__.
        patchJump(skip_jump, program_.code.size());
    }

    // Generate regular methods
    for (auto& method : node.methods) {
        if (!method) {
            continue;
        }

        std::string previous_method = current_method_;
        current_method_ = method->name;

        std::string symbol = node.name + "." + method->name;

        // Emit a JUMP placeholder so the method body is skipped during linear
        // execution. METHOD_CALL jumps directly to the body via function_table.
        std::size_t skip_jump = emitJump(OpCode::JUMP);

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

        // Patch the skip JUMP to land just past this method's body.
        patchJump(skip_jump, program_.code.size());

        current_method_ = previous_method;
    }

    current_type_name_ = previous_type;
}
void CodeGenerator::visit(ProtocolDef&) {}
void CodeGenerator::visit(ProtocolMethodSig&) {}

void CodeGenerator::visit(NewExpr& node) {
    // 1. Create empty object (pushes obj on stack)
    emit(Instruction::New(node.type_name, 0));

    // 2. Evaluate constructor args (stack: [obj, arg1, ..., argN])
    for (auto& arg : node.args) {
        if (arg) {
            arg->accept(*this);
        }
    }

    // 3. Call __init__ via METHOD_CALL (pops args + obj, sets self=obj, calls init)
    emit(Instruction::MethodCall("__init__", static_cast<int>(node.args.size())));
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

    std::string target;
    if (!current_method_.empty() && !current_type_name_.empty()) {
        // Walk the ancestor chain to find the closest ancestor that implements this method.
        auto anc_it = program_.type_ancestors.find(current_type_name_);
        if (anc_it != program_.type_ancestors.end()) {
            target = "Object." + current_method_;  // fallback if no ancestor defines it
            for (const auto& ancestor : anc_it->second) {
                std::string candidate = ancestor + "." + current_method_;
                if (program_.function_table.find(candidate) != program_.function_table.end()) {
                    target = candidate;
                    break;
                }
            }
        } else {
            target = "Object." + current_method_;
        }
    } else {
        target = "base";
    }
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
    // Push elements in source order. NEW_VECTOR pops them (reversing once)
    // and reverses again internally, so forward push yields source order.
    for (auto& element : node.elements) {
        if (element) {
            element->accept(*this);
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
    // Build parent map for ancestor chain computation
    std::unordered_map<std::string, std::string> parent_map;
    for (auto& type_def : node.types) {
        if (type_def) {
            parent_map[type_def->name] = type_def->parent_name;
        }
    }
    // Built-in type hierarchy
    parent_map["Number"] = "Object";
    parent_map["String"] = "Object";
    parent_map["Boolean"] = "Object";
    parent_map["Object"] = "";

    // Store parent map for BaseCall resolution
    parent_map_ = parent_map;

    // Compute full ancestor chains for each user-defined type
    for (auto& type_def : node.types) {
        if (type_def) {
            std::vector<std::string> ancestors;
            std::string current = type_def->parent_name;
            if (current.empty()) {
                current = "Object";
            }
            while (!current.empty()) {
                ancestors.push_back(current);
                auto it = parent_map.find(current);
                if (it == parent_map.end() || it->second.empty()) {
                    break;
                }
                current = it->second;
            }
            program_.type_ancestors[type_def->name] = ancestors;
        }
    }

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

void CodeGenerator::visit(LambdaExpr& node) {
    // 1. Create empty object (the anonymous lambda TypeDef)
    emit(Instruction::New(node.generated_type_name, 0));

    // 2. Evaluate captured variables as args
    for (const auto& var : node.captured_vars) {
        emitLoad(var);
    }

    // 3. Call __init__ to store captured vars as attributes
    emit(Instruction::MethodCall("__init__", static_cast<int>(node.captured_vars.size())));
}
