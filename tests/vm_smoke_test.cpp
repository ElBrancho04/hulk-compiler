#include <functional>
#include <iostream>

#include "bytecode.hpp"
#include "runtime_error.hpp"
#include "vm.hpp"

namespace {
Value run_program(BytecodeProgram& program) {
    VM vm;
    vm.execute(program);
    return vm.popStack();
}

bool expect_runtime_error(const std::string& label, const std::function<void()>& fn) {
    try {
        fn();
    } catch (const RuntimeError&) {
        std::cout << "[OK] " << label << " raised RuntimeError" << std::endl;
        return true;
    }

    std::cerr << "[FAIL] " << label << " did not raise RuntimeError" << std::endl;
    return false;
}
}

int main() {
    try {
        // 1 + 2
        {
            BytecodeProgram program;
            auto c1 = program.addConstant(Value::Number(1.0));
            auto c2 = program.addConstant(Value::Number(2.0));
            program.addInstruction(Instruction::PushConst(static_cast<int>(c1)));
            program.addInstruction(Instruction::PushConst(static_cast<int>(c2)));
            program.addInstruction(Instruction(OpCode::ADD));
            program.addInstruction(Instruction(OpCode::HALT));
            Value result = run_program(program);
            if (result.type != ValueType::Number || result.number_value != 3.0) {
                std::cerr << "[FAIL] basic arithmetic" << std::endl;
                return 1;
            }
        }

        // CALL/RETURN
        {
            BytecodeProgram program;
            auto c1 = program.addConstant(Value::Number(1.0));
            auto c2 = program.addConstant(Value::Number(2.0));
            program.addFunctionSymbol("add1", 1);
            program.addInstruction(Instruction::Jump(6));
            program.addInstruction(Instruction::Label(1));
            program.addInstruction(Instruction::Store("x"));
            program.addInstruction(Instruction::Load("x"));
            program.addInstruction(Instruction::PushConst(static_cast<int>(c1)));
            program.addInstruction(Instruction(OpCode::ADD));
            program.addInstruction(Instruction(OpCode::RETURN));
            program.addInstruction(Instruction::Label(7));
            program.addInstruction(Instruction::PushConst(static_cast<int>(c2)));
            program.addInstruction(Instruction::Call("add1", 1));
            program.addInstruction(Instruction(OpCode::HALT));
            Value result = run_program(program);
            if (result.type != ValueType::Number || result.number_value != 3.0) {
                std::cerr << "[FAIL] call/return" << std::endl;
                return 1;
            }
        }

        // NEW_VECTOR + VECTOR_INDEX
        {
            BytecodeProgram program;
            auto v1 = program.addConstant(Value::Number(10.0));
            auto v2 = program.addConstant(Value::Number(20.0));
            auto idx = program.addConstant(Value::Number(1.0));
            program.addInstruction(Instruction::PushConst(static_cast<int>(v1)));
            program.addInstruction(Instruction::PushConst(static_cast<int>(v2)));
            program.addInstruction(Instruction::NewVector(2));
            program.addInstruction(Instruction::PushConst(static_cast<int>(idx)));
            program.addInstruction(Instruction(OpCode::VECTOR_INDEX));
            program.addInstruction(Instruction(OpCode::HALT));
            Value result = run_program(program);
            if (result.type != ValueType::Number || result.number_value != 20.0) {
                std::cerr << "[FAIL] vector index" << std::endl;
                return 1;
            }
        }

        // RANGE + ITER
        {
            BytecodeProgram program;
            auto start = program.addConstant(Value::Number(1.0));
            auto end = program.addConstant(Value::Number(3.0));
            program.addInstruction(Instruction::PushConst(static_cast<int>(start)));
            program.addInstruction(Instruction::PushConst(static_cast<int>(end)));
            program.addInstruction(Instruction(OpCode::RANGE));
            program.addInstruction(Instruction::Store("r"));
            program.addInstruction(Instruction::Load("r"));
            program.addInstruction(Instruction(OpCode::ITER_NEXT));
            program.addInstruction(Instruction(OpCode::POP));
            program.addInstruction(Instruction::Load("r"));
            program.addInstruction(Instruction(OpCode::ITER_CURRENT));
            program.addInstruction(Instruction(OpCode::HALT));
            Value result = run_program(program);
            if (result.type != ValueType::Number || result.number_value != 1.0) {
                std::cerr << "[FAIL] range iteration" << std::endl;
                return 1;
            }
        }

        // Errors: invalid AS and division by zero
        bool as_error = expect_runtime_error("invalid AS", []() {
            BytecodeProgram program;
            program.addInstruction(Instruction::New("Foo", 0));
            program.addInstruction(Instruction::As("Bar"));
            program.addInstruction(Instruction(OpCode::HALT));
            run_program(program);
        });

        bool div_error = expect_runtime_error("division by zero", []() {
            BytecodeProgram program;
            auto n1 = program.addConstant(Value::Number(1.0));
            auto n0 = program.addConstant(Value::Number(0.0));
            program.addInstruction(Instruction::PushConst(static_cast<int>(n1)));
            program.addInstruction(Instruction::PushConst(static_cast<int>(n0)));
            program.addInstruction(Instruction(OpCode::DIV));
            program.addInstruction(Instruction(OpCode::HALT));
            run_program(program);
        });

        if (!as_error || !div_error) {
            return 1;
        }

        std::cout << "VM smoke test passed." << std::endl;
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "VM smoke test failed: " << ex.what() << std::endl;
        return 1;
    }
}
