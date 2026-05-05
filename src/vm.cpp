#include "vm.hpp"

#include <algorithm>
#include <cmath>

#include "builtins.hpp"
#include "hulk_object.hpp"
#include "runtime_error.hpp"

VM::VM()
        : global_env_(create_global_env()),
            current_env_(global_env_) {}

void VM::execute(BytecodeProgram& program) {
    constants_ = &program.constants;
    ip_ = 0;
    stack_.clear();
    call_stack_.clear();
    current_env_ = global_env_;
    current_self_ = Value::Null();
    const auto& code = program.code;

    auto popValue = [&]() -> Value {
        if (stack_.empty()) {
            throw RuntimeError("VM stack underflow");
        }
        Value value = stack_.back();
        stack_.pop_back();
        return value;
    };

    auto popNumber = [&](const std::string& op) -> double {
        Value value = popValue();
        if (value.type != ValueType::Number) {
            throw RuntimeError("Expected Number for " + op);
        }
        return value.number_value;
    };

    auto popBoolean = [&](const std::string& op) -> bool {
        Value value = popValue();
        if (value.type != ValueType::Boolean) {
            throw RuntimeError("Expected Boolean for " + op);
        }
        return value.bool_value;
    };

    auto popString = [&](const std::string& op) -> std::string {
        Value value = popValue();
        if (value.type != ValueType::String) {
            throw RuntimeError("Expected String for " + op);
        }
        return value.string_value;
    };

    auto popObject = [&](const std::string& op) -> std::shared_ptr<HulkObject> {
        Value value = popValue();
        if (value.type != ValueType::Object || !value.object_value) {
            throw RuntimeError("Expected Object for " + op);
        }
        return value.object_value;
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
            case OpCode::ADD: {
                double rhs = popNumber("ADD");
                double lhs = popNumber("ADD");
                stack_.push_back(Value::Number(lhs + rhs));
                break;
            }
            case OpCode::SUB: {
                double rhs = popNumber("SUB");
                double lhs = popNumber("SUB");
                stack_.push_back(Value::Number(lhs - rhs));
                break;
            }
            case OpCode::MUL: {
                double rhs = popNumber("MUL");
                double lhs = popNumber("MUL");
                stack_.push_back(Value::Number(lhs * rhs));
                break;
            }
            case OpCode::DIV: {
                double rhs = popNumber("DIV");
                double lhs = popNumber("DIV");
                if (rhs == 0.0) {
                    throw RuntimeError("Division by zero");
                }
                stack_.push_back(Value::Number(lhs / rhs));
                break;
            }
            case OpCode::POW: {
                double rhs = popNumber("POW");
                double lhs = popNumber("POW");
                stack_.push_back(Value::Number(std::pow(lhs, rhs)));
                break;
            }
            case OpCode::NEG: {
                double value = popNumber("NEG");
                stack_.push_back(Value::Number(-value));
                break;
            }
            case OpCode::NOT: {
                bool value = popBoolean("NOT");
                stack_.push_back(Value::Boolean(!value));
                break;
            }
            case OpCode::AND: {
                bool rhs = popBoolean("AND");
                bool lhs = popBoolean("AND");
                stack_.push_back(Value::Boolean(lhs && rhs));
                break;
            }
            case OpCode::OR: {
                bool rhs = popBoolean("OR");
                bool lhs = popBoolean("OR");
                stack_.push_back(Value::Boolean(lhs || rhs));
                break;
            }
            case OpCode::CONCAT: {
                std::string rhs = popString("CONCAT");
                std::string lhs = popString("CONCAT");
                stack_.push_back(Value::String(lhs + rhs));
                break;
            }
            case OpCode::CONCAT_SPACE: {
                std::string rhs = popString("CONCAT_SPACE");
                std::string lhs = popString("CONCAT_SPACE");
                stack_.push_back(Value::String(lhs + " " + rhs));
                break;
            }
            case OpCode::CMP_EQ: {
                Value rhs = popValue();
                Value lhs = popValue();
                stack_.push_back(Value::Boolean(lhs == rhs));
                break;
            }
            case OpCode::CMP_NEQ: {
                Value rhs = popValue();
                Value lhs = popValue();
                stack_.push_back(Value::Boolean(lhs != rhs));
                break;
            }
            case OpCode::CMP_LT: {
                double rhs = popNumber("CMP_LT");
                double lhs = popNumber("CMP_LT");
                stack_.push_back(Value::Boolean(lhs < rhs));
                break;
            }
            case OpCode::CMP_GT: {
                double rhs = popNumber("CMP_GT");
                double lhs = popNumber("CMP_GT");
                stack_.push_back(Value::Boolean(lhs > rhs));
                break;
            }
            case OpCode::CMP_LE: {
                double rhs = popNumber("CMP_LE");
                double lhs = popNumber("CMP_LE");
                stack_.push_back(Value::Boolean(lhs <= rhs));
                break;
            }
            case OpCode::CMP_GE: {
                double rhs = popNumber("CMP_GE");
                double lhs = popNumber("CMP_GE");
                stack_.push_back(Value::Boolean(lhs >= rhs));
                break;
            }
            case OpCode::JUMP: {
                ip_ = static_cast<std::size_t>(static_cast<long long>(ip_ + 1) + inst.offset);
                advance_ip = false;
                break;
            }
            case OpCode::JUMP_IF_FALSE: {
                bool condition = popBoolean("JUMP_IF_FALSE");
                if (!condition) {
                    ip_ = static_cast<std::size_t>(static_cast<long long>(ip_ + 1) + inst.offset);
                    advance_ip = false;
                }
                break;
            }
            case OpCode::JUMP_IF_TRUE: {
                bool condition = popBoolean("JUMP_IF_TRUE");
                if (condition) {
                    ip_ = static_cast<std::size_t>(static_cast<long long>(ip_ + 1) + inst.offset);
                    advance_ip = false;
                }
                break;
            }
            case OpCode::CALL: {
                if (inst.count < 0) {
                    throw RuntimeError("Invalid CALL arg count");
                }
                std::size_t argc = static_cast<std::size_t>(inst.count);
                std::vector<Value> args_rev;
                args_rev.reserve(argc);
                for (std::size_t i = 0; i < argc; ++i) {
                    args_rev.push_back(popValue());
                }
                std::vector<Value> args(args_rev.rbegin(), args_rev.rend());

                auto it = program.function_table.find(inst.name);
                if (it != program.function_table.end()) {
                    for (const auto& value : args_rev) {
                        stack_.push_back(value);
                    }
                    call_stack_.push_back({ip_ + 1, current_env_, current_self_});
                    current_env_ = std::make_shared<Environment>(global_env_);
                    current_self_ = Value::Null();
                    ip_ = it->second;
                    advance_ip = false;
                    break;
                }

                Value builtin_value = global_env_->get(inst.name);
                if (builtin_value.type != ValueType::Object || !builtin_value.object_value) {
                    throw RuntimeError("Undefined function: " + inst.name);
                }
                auto builtin = std::dynamic_pointer_cast<NativeFunctionObject>(builtin_value.object_value);
                if (!builtin) {
                    throw RuntimeError("Symbol is not callable: " + inst.name);
                }
                Value result = builtin->function(args);
                stack_.push_back(result);
                break;
            }
            case OpCode::RETURN: {
                Value return_value = popValue();
                if (call_stack_.empty()) {
                    throw RuntimeError("RETURN with empty call stack");
                }
                CallFrame frame = call_stack_.back();
                call_stack_.pop_back();
                current_env_ = frame.env;
                current_self_ = frame.self;
                ip_ = frame.return_ip;
                stack_.push_back(return_value);
                advance_ip = false;
                break;
            }
            case OpCode::NEW: {
                auto obj = std::make_shared<HulkObject>(inst.name);
                stack_.push_back(Value::Object(obj));
                break;
            }
            case OpCode::GET_ATTR: {
                auto obj = popObject("GET_ATTR");
                if (!obj->hasAttribute(inst.name)) {
                    throw RuntimeError("Unknown attribute: " + inst.name);
                }
                stack_.push_back(obj->getAttribute(inst.name));
                break;
            }
            case OpCode::SET_ATTR: {
                Value value = popValue();
                auto obj = popObject("SET_ATTR");
                obj->setAttribute(inst.name, value);
                stack_.push_back(Value::Object(obj));
                break;
            }
            case OpCode::SELF: {
                stack_.push_back(current_self_);
                break;
            }
            case OpCode::METHOD_CALL: {
                if (inst.count < 0) {
                    throw RuntimeError("Invalid METHOD_CALL arg count");
                }
                std::size_t argc = static_cast<std::size_t>(inst.count);
                std::vector<Value> args_rev;
                args_rev.reserve(argc);
                for (std::size_t i = 0; i < argc; ++i) {
                    args_rev.push_back(popValue());
                }
                auto obj = popObject("METHOD_CALL");
                std::vector<Value> args(args_rev.rbegin(), args_rev.rend());

                std::string symbol = obj->type_name + "." + inst.name;
                auto it = program.function_table.find(symbol);
                if (it == program.function_table.end()) {
                    throw RuntimeError("Unknown method: " + symbol);
                }

                for (const auto& value : args_rev) {
                    stack_.push_back(value);
                }
                call_stack_.push_back({ip_ + 1, current_env_, current_self_});
                current_env_ = std::make_shared<Environment>(global_env_);
                current_self_ = Value::Object(obj);
                ip_ = it->second;
                advance_ip = false;
                break;
            }
            case OpCode::BASE_CALL: {
                if (inst.count < 0) {
                    throw RuntimeError("Invalid BASE_CALL arg count");
                }
                std::size_t argc = static_cast<std::size_t>(inst.count);
                std::vector<Value> args_rev;
                args_rev.reserve(argc);
                for (std::size_t i = 0; i < argc; ++i) {
                    args_rev.push_back(popValue());
                }
                std::vector<Value> args(args_rev.rbegin(), args_rev.rend());

                auto it = program.function_table.find(inst.name);
                if (it == program.function_table.end()) {
                    throw RuntimeError("Unknown base method: " + inst.name);
                }

                for (const auto& value : args_rev) {
                    stack_.push_back(value);
                }
                call_stack_.push_back({ip_ + 1, current_env_, current_self_});
                current_env_ = std::make_shared<Environment>(global_env_);
                ip_ = it->second;
                advance_ip = false;
                break;
            }
            case OpCode::IS: {
                auto obj = popObject("IS");
                stack_.push_back(Value::Boolean(obj->type_name == inst.name));
                break;
            }
            case OpCode::AS: {
                auto obj = popObject("AS");
                if (obj->type_name != inst.name) {
                    throw RuntimeError("Invalid downcast to " + inst.name);
                }
                stack_.push_back(Value::Object(obj));
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
