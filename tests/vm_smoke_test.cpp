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

        // VECTOR ITER: iterate [10, 20, 30], check first current == 10
        {
            BytecodeProgram program;
            auto c10 = program.addConstant(Value::Number(10.0));
            auto c20 = program.addConstant(Value::Number(20.0));
            auto c30 = program.addConstant(Value::Number(30.0));
            program.addInstruction(Instruction::PushConst(static_cast<int>(c10)));
            program.addInstruction(Instruction::PushConst(static_cast<int>(c20)));
            program.addInstruction(Instruction::PushConst(static_cast<int>(c30)));
            program.addInstruction(Instruction::NewVector(3));
            program.addInstruction(Instruction::Store("v"));
            // next() → true
            program.addInstruction(Instruction::Load("v"));
            program.addInstruction(Instruction(OpCode::ITER_NEXT));
            program.addInstruction(Instruction(OpCode::POP));
            // current() → 10
            program.addInstruction(Instruction::Load("v"));
            program.addInstruction(Instruction(OpCode::ITER_CURRENT));
            program.addInstruction(Instruction(OpCode::HALT));
            Value result = run_program(program);
            if (result.type != ValueType::Number || result.number_value != 10.0) {
                std::cerr << "[FAIL] vector iter first element" << std::endl;
                return 1;
            }
            std::cout << "[OK] vector iter first element" << std::endl;
        }

        // VECTOR ITER: exhaust [5, 6], check next() returns false at end
        {
            BytecodeProgram program;
            auto c5 = program.addConstant(Value::Number(5.0));
            auto c6 = program.addConstant(Value::Number(6.0));
            program.addInstruction(Instruction::PushConst(static_cast<int>(c5)));
            program.addInstruction(Instruction::PushConst(static_cast<int>(c6)));
            program.addInstruction(Instruction::NewVector(2));
            program.addInstruction(Instruction::Store("v2"));
            // next() x2 (both true)
            program.addInstruction(Instruction::Load("v2"));
            program.addInstruction(Instruction(OpCode::ITER_NEXT));
            program.addInstruction(Instruction(OpCode::POP));
            program.addInstruction(Instruction::Load("v2"));
            program.addInstruction(Instruction(OpCode::ITER_NEXT));
            program.addInstruction(Instruction(OpCode::POP));
            // next() x1 → false (past end)
            program.addInstruction(Instruction::Load("v2"));
            program.addInstruction(Instruction(OpCode::ITER_NEXT));
            program.addInstruction(Instruction(OpCode::HALT));
            Value result = run_program(program);
            if (result.type != ValueType::Boolean || result.bool_value != false) {
                std::cerr << "[FAIL] vector iter exhaustion" << std::endl;
                return 1;
            }
            std::cout << "[OK] vector iter exhaustion" << std::endl;
        }

        // FOR LOOP OVER VECTOR: for(x in [1,2,3]) x → result = 3 (last body value)
        // Mirrors the bytecode pattern emitted by the fixed visit(ForExpr)
        {
            BytecodeProgram program;
            auto null_c = program.addConstant(Value::Null());
            auto c1 = program.addConstant(Value::Number(1.0));
            auto c2 = program.addConstant(Value::Number(2.0));
            auto c3 = program.addConstant(Value::Number(3.0));

            // ip 0: PUSH_CONST null → STORE _result
            program.addInstruction(Instruction::PushConst(static_cast<int>(null_c)));
            program.addInstruction(Instruction::Store("_result"));

            // ip 2-6: build vector [1,2,3], STORE _iter
            program.addInstruction(Instruction::PushConst(static_cast<int>(c1)));
            program.addInstruction(Instruction::PushConst(static_cast<int>(c2)));
            program.addInstruction(Instruction::PushConst(static_cast<int>(c3)));
            program.addInstruction(Instruction::NewVector(3));
            program.addInstruction(Instruction::Store("_iter"));

            // ip 7: loop_start — LOAD _iter, ITER_NEXT
            program.addInstruction(Instruction::Load("_iter"));   // ip 7
            program.addInstruction(Instruction(OpCode::ITER_NEXT)); // ip 8
            // ip 9: JUMP_IF_FALSE exit (ip 16) → offset = 16 - 10 = 6
            program.addInstruction(Instruction::JumpIfFalse(6));   // ip 9

            // ip 10-12: LOAD _iter, ITER_CURRENT, STORE x
            program.addInstruction(Instruction::Load("_iter"));    // ip 10
            program.addInstruction(Instruction(OpCode::ITER_CURRENT)); // ip 11
            program.addInstruction(Instruction::Store("x"));       // ip 12

            // ip 13-14: body (LOAD x) → STORE _result
            program.addInstruction(Instruction::Load("x"));        // ip 13
            program.addInstruction(Instruction::Store("_result")); // ip 14

            // ip 15: JUMP back to loop_start (ip 7) → offset = 7 - 16 = -9
            program.addInstruction(Instruction::Jump(-9));         // ip 15

            // ip 16: exit → LOAD _result, HALT
            program.addInstruction(Instruction::Load("_result"));  // ip 16
            program.addInstruction(Instruction(OpCode::HALT));     // ip 17

            Value result = run_program(program);
            if (result.type != ValueType::Number || result.number_value != 3.0) {
                std::cerr << "[FAIL] for-loop over vector (expected 3, got "
                          << result.number_value << ")" << std::endl;
                return 1;
            }
            std::cout << "[OK] for-loop over vector (last element = 3)" << std::endl;
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
