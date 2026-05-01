#include <iostream>

#include "bytecode.hpp"

int main() {
    try {
        BytecodeProgram program;

        std::size_t c1 = program.addConstant(Value::Number(1.0));
        std::size_t c2 = program.addConstant(Value::String("hola"));
        std::size_t c3 = program.addConstant(Value::Boolean(true));

        program.addInstruction(Instruction::PushConst(static_cast<int>(c1)));
        program.addInstruction(Instruction::PushConst(static_cast<int>(c2)));
        program.addInstruction(Instruction::PushConst(static_cast<int>(c3)));
        program.addInstruction(Instruction::Load("x"));
        program.addInstruction(Instruction::Store("x"));
        program.addInstruction(Instruction::Assign("x"));
        program.addInstruction(Instruction(OpCode::POP));
    program.addInstruction(Instruction(OpCode::POP_SCOPE));
        program.addInstruction(Instruction(OpCode::ADD));
        program.addInstruction(Instruction(OpCode::SUB));
        program.addInstruction(Instruction(OpCode::MUL));
        program.addInstruction(Instruction(OpCode::DIV));
        program.addInstruction(Instruction(OpCode::POW));
        program.addInstruction(Instruction(OpCode::NEG));
        program.addInstruction(Instruction(OpCode::NOT));
        program.addInstruction(Instruction(OpCode::AND));
        program.addInstruction(Instruction(OpCode::OR));
        program.addInstruction(Instruction(OpCode::CONCAT));
        program.addInstruction(Instruction(OpCode::CONCAT_SPACE));
        program.addInstruction(Instruction(OpCode::CMP_EQ));
        program.addInstruction(Instruction(OpCode::CMP_NEQ));
        program.addInstruction(Instruction(OpCode::CMP_LT));
        program.addInstruction(Instruction(OpCode::CMP_GT));
        program.addInstruction(Instruction(OpCode::CMP_LE));
        program.addInstruction(Instruction(OpCode::CMP_GE));
        program.addInstruction(Instruction::Jump(5));
        program.addInstruction(Instruction::JumpIfFalse(2));
        program.addInstruction(Instruction::JumpIfTrue(3));
        program.addInstruction(Instruction::Label(10));
        program.addInstruction(Instruction::Call("foo", 2));
        program.addInstruction(Instruction(OpCode::RETURN));
        program.addInstruction(Instruction::New("Point", 2));
        program.addInstruction(Instruction::GetAttr("x"));
        program.addInstruction(Instruction::SetAttr("x"));
        program.addInstruction(Instruction(OpCode::SELF));
        program.addInstruction(Instruction::BaseCall("bar", 1));
        program.addInstruction(Instruction::Is("Point"));
        program.addInstruction(Instruction::As("Point"));
        program.addInstruction(Instruction::NewVector(3));
    program.addInstruction(Instruction(OpCode::VECTOR_INIT));
    program.addInstruction(Instruction(OpCode::VECTOR_PUSH));
        program.addInstruction(Instruction(OpCode::VECTOR_INDEX));
        program.addInstruction(Instruction(OpCode::ITER_NEXT));
        program.addInstruction(Instruction(OpCode::ITER_CURRENT));
        program.addInstruction(Instruction(OpCode::RANGE));
        program.addInstruction(Instruction(OpCode::SIZE));

        program.addFunctionSymbol("foo", 0);

        std::cout << to_string(program) << std::endl;
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Bytecode smoke test failed: " << ex.what() << std::endl;
        return 1;
    }
}
