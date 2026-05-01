#ifndef BYTECODE_HPP
#define BYTECODE_HPP

#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "value.hpp"
#include "value_string.hpp"

enum class OpCode {
    PUSH_CONST,
    LOAD,
    STORE,
    ASSIGN,
    POP,
    ADD,
    SUB,
    MUL,
    DIV,
    POW,
    NEG,
    NOT,
    AND,
    OR,
    CONCAT,
    CONCAT_SPACE,
    CMP_EQ,
    CMP_NEQ,
    CMP_LT,
    CMP_GT,
    CMP_LE,
    CMP_GE,
    JUMP,
    JUMP_IF_FALSE,
    JUMP_IF_TRUE,
    LABEL,
    CALL,
    RETURN,
    HALT,
    NEW,
    GET_ATTR,
    SET_ATTR,
    SELF,
    BASE_CALL,
    IS,
    AS,
    NEW_VECTOR,
    VECTOR_INDEX,
    ITER_NEXT,
    ITER_CURRENT,
    RANGE,
    SIZE
};

struct Instruction {
    OpCode opcode;
    int index = 0;
    int offset = 0;
    int count = 0;
    std::string name;

    explicit Instruction(OpCode opcode)
        : opcode(opcode) {}

    static Instruction PushConst(int index) {
        Instruction inst(OpCode::PUSH_CONST);
        inst.index = index;
        return inst;
    }

    static Instruction Load(std::string name) {
        Instruction inst(OpCode::LOAD);
        inst.name = std::move(name);
        return inst;
    }

    static Instruction Store(std::string name) {
        Instruction inst(OpCode::STORE);
        inst.name = std::move(name);
        return inst;
    }

    static Instruction Assign(std::string name) {
        Instruction inst(OpCode::ASSIGN);
        inst.name = std::move(name);
        return inst;
    }

    static Instruction Jump(int offset) {
        Instruction inst(OpCode::JUMP);
        inst.offset = offset;
        return inst;
    }

    static Instruction JumpIfFalse(int offset) {
        Instruction inst(OpCode::JUMP_IF_FALSE);
        inst.offset = offset;
        return inst;
    }

    static Instruction JumpIfTrue(int offset) {
        Instruction inst(OpCode::JUMP_IF_TRUE);
        inst.offset = offset;
        return inst;
    }

    static Instruction Call(std::string name, int num_args) {
        Instruction inst(OpCode::CALL);
        inst.name = std::move(name);
        inst.count = num_args;
        return inst;
    }

    static Instruction New(std::string name, int num_args) {
        Instruction inst(OpCode::NEW);
        inst.name = std::move(name);
        inst.count = num_args;
        return inst;
    }

    static Instruction GetAttr(std::string name) {
        Instruction inst(OpCode::GET_ATTR);
        inst.name = std::move(name);
        return inst;
    }

    static Instruction SetAttr(std::string name) {
        Instruction inst(OpCode::SET_ATTR);
        inst.name = std::move(name);
        return inst;
    }

    static Instruction BaseCall(std::string name, int num_args) {
        Instruction inst(OpCode::BASE_CALL);
        inst.name = std::move(name);
        inst.count = num_args;
        return inst;
    }

    static Instruction Is(std::string name) {
        Instruction inst(OpCode::IS);
        inst.name = std::move(name);
        return inst;
    }

    static Instruction As(std::string name) {
        Instruction inst(OpCode::AS);
        inst.name = std::move(name);
        return inst;
    }

    static Instruction NewVector(int count) {
        Instruction inst(OpCode::NEW_VECTOR);
        inst.count = count;
        return inst;
    }

    static Instruction Label(int index) {
        Instruction inst(OpCode::LABEL);
        inst.index = index;
        return inst;
    }
};

inline std::string to_string(OpCode opcode) {
    switch (opcode) {
        case OpCode::PUSH_CONST: return "PUSH_CONST";
        case OpCode::LOAD: return "LOAD";
        case OpCode::STORE: return "STORE";
        case OpCode::ASSIGN: return "ASSIGN";
        case OpCode::POP: return "POP";
        case OpCode::ADD: return "ADD";
        case OpCode::SUB: return "SUB";
        case OpCode::MUL: return "MUL";
        case OpCode::DIV: return "DIV";
        case OpCode::POW: return "POW";
        case OpCode::NEG: return "NEG";
        case OpCode::NOT: return "NOT";
        case OpCode::AND: return "AND";
        case OpCode::OR: return "OR";
        case OpCode::CONCAT: return "CONCAT";
        case OpCode::CONCAT_SPACE: return "CONCAT_SPACE";
        case OpCode::CMP_EQ: return "CMP_EQ";
        case OpCode::CMP_NEQ: return "CMP_NEQ";
        case OpCode::CMP_LT: return "CMP_LT";
        case OpCode::CMP_GT: return "CMP_GT";
        case OpCode::CMP_LE: return "CMP_LE";
        case OpCode::CMP_GE: return "CMP_GE";
        case OpCode::JUMP: return "JUMP";
        case OpCode::JUMP_IF_FALSE: return "JUMP_IF_FALSE";
        case OpCode::JUMP_IF_TRUE: return "JUMP_IF_TRUE";
        case OpCode::LABEL: return "LABEL";
        case OpCode::CALL: return "CALL";
        case OpCode::RETURN: return "RETURN";
        case OpCode::HALT: return "HALT";
        case OpCode::NEW: return "NEW";
        case OpCode::GET_ATTR: return "GET_ATTR";
        case OpCode::SET_ATTR: return "SET_ATTR";
        case OpCode::SELF: return "SELF";
        case OpCode::BASE_CALL: return "BASE_CALL";
    case OpCode::IS: return "IS";
    case OpCode::AS: return "AS";
        case OpCode::NEW_VECTOR: return "NEW_VECTOR";
        case OpCode::VECTOR_INDEX: return "VECTOR_INDEX";
        case OpCode::ITER_NEXT: return "ITER_NEXT";
        case OpCode::ITER_CURRENT: return "ITER_CURRENT";
        case OpCode::RANGE: return "RANGE";
        case OpCode::SIZE: return "SIZE";
    }

    return "UNKNOWN";
}

inline std::string to_string(const Instruction& inst) {
    std::ostringstream out;
    out << to_string(inst.opcode);

    switch (inst.opcode) {
        case OpCode::PUSH_CONST:
        case OpCode::LABEL:
            out << " " << inst.index;
            break;
        case OpCode::LOAD:
        case OpCode::STORE:
        case OpCode::ASSIGN:
        case OpCode::GET_ATTR:
        case OpCode::SET_ATTR:
        case OpCode::IS:
        case OpCode::AS:
            out << " " << inst.name;
            break;
        case OpCode::JUMP:
        case OpCode::JUMP_IF_FALSE:
        case OpCode::JUMP_IF_TRUE:
            out << " " << inst.offset;
            break;
        case OpCode::CALL:
        case OpCode::NEW:
        case OpCode::BASE_CALL:
            out << " " << inst.name << " " << inst.count;
            break;
        case OpCode::NEW_VECTOR:
            out << " " << inst.count;
            break;
        default:
            break;
    }

    return out.str();
}

struct BytecodeProgram {
    std::vector<Instruction> code;
    std::vector<Value> constants;
    std::unordered_map<std::string, std::size_t> function_table;

    std::size_t addConstant(const Value& value, bool dedupe = true) {
        if (dedupe) {
            for (std::size_t i = 0; i < constants.size(); ++i) {
                if (constants[i] == value) {
                    return i;
                }
            }
        }

        constants.push_back(value);
        return constants.size() - 1;
    }

    std::size_t addInstruction(const Instruction& inst) {
        code.push_back(inst);
        return code.size() - 1;
    }

    void addFunctionSymbol(const std::string& name, std::size_t index) {
        function_table[name] = index;
    }
};

inline std::string to_string(const BytecodeProgram& program) {
    std::ostringstream out;
    out << "== CONSTANTS ==\n";
    for (std::size_t i = 0; i < program.constants.size(); ++i) {
        out << i << ": " << to_string(program.constants[i]) << "\n";
    }

    out << "== CODE ==\n";
    for (std::size_t i = 0; i < program.code.size(); ++i) {
        out << i << ": " << to_string(program.code[i]) << "\n";
    }

    out << "== FUNCTIONS ==\n";
    for (const auto& entry : program.function_table) {
        out << entry.first << " -> " << entry.second << "\n";
    }

    return out.str();
}

#endif
