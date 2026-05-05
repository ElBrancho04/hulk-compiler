#include "vm.hpp"

#include "runtime_error.hpp"

VM::VM()
    : global_env_(std::make_shared<Environment>()),
      current_env_(global_env_) {}

void VM::execute(BytecodeProgram& program) {
    constants_ = &program.constants;
    ip_ = 0;
    stack_.clear();
    call_stack_.clear();
    current_env_ = global_env_;
    const auto& code = program.code;

    auto popValue = [&]() -> Value {
        if (stack_.empty()) {
            throw RuntimeError("VM stack underflow");
        }
        Value value = stack_.back();
        stack_.pop_back();
        return value;
    };

    while (ip_ < code.size()) {
        const Instruction& inst = code[ip_];
        bool advance_ip = true;

        switch (inst.opcode) {
            case OpCode::PUSH_CONST: {
                if (!constants_ || inst.index < 0 || static_cast<std::size_t>(inst.index) >= constants_->size()) {
                    throw RuntimeError("Invalid constant index");
                }
                stack_.push_back((*constants_)[static_cast<std::size_t>(inst.index)]);
                break;
            }
            case OpCode::LOAD: {
                stack_.push_back(current_env_->get(inst.name));
                break;
            }
            case OpCode::STORE: {
                Value value = popValue();
                current_env_->define(inst.name, value);
                break;
            }
            case OpCode::ASSIGN: {
                Value value = popValue();
                current_env_->assign(inst.name, value);
                break;
            }
            case OpCode::POP: {
                popValue();
                break;
            }
            case OpCode::POP_SCOPE: {
                if (current_env_ && current_env_->parent) {
                    current_env_ = current_env_->parent;
                }
                break;
            }
            default:
                break;
        }

        if (advance_ip) {
            ++ip_;
        }
    }
}
