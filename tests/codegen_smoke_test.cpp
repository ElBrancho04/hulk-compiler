#include <iostream>
#include <memory>

#include "ast.hpp"
#include "code_generator.hpp"
#include "bytecode.hpp"

int main() {
    try {
        std::vector<Parameter> params;
        params.emplace_back("x");

        auto func_body = std::make_unique<BinaryExpr>(
            "+",
            std::make_unique<VarRef>("x", 1),
            std::make_unique<NumberLiteral>(1.0, 1),
            1
        );

        auto func = std::make_unique<FuncDef>(
            "add1",
            params,
            "",
            std::move(func_body),
            1
        );

        std::vector<std::unique_ptr<FuncDef>> functions;
        functions.push_back(std::move(func));

        std::vector<std::unique_ptr<TypeDef>> types;
        std::vector<std::unique_ptr<ProtocolDef>> protocols;

        std::vector<std::unique_ptr<Expr>> call_args;
        call_args.push_back(std::make_unique<NumberLiteral>(2.0, 1));
        auto global_expr = std::make_unique<FuncCall>("add1", std::move(call_args), 1);

        Program program(
            std::move(types),
            std::move(protocols),
            std::move(functions),
            std::move(global_expr),
            1
        );

        BytecodeProgram bytecode;
        CodeGenerator generator(bytecode);
        program.accept(generator);

        std::cout << to_string(bytecode) << std::endl;

        if (bytecode.constants.empty()) {
            std::cerr << "[FAIL] constants were not registered" << std::endl;
            return 1;
        }

        bool has_function = bytecode.function_table.find("add1") != bytecode.function_table.end();
        if (!has_function) {
            std::cerr << "[FAIL] function symbol missing" << std::endl;
            return 1;
        }

        for (std::size_t i = 0; i < bytecode.code.size(); ++i) {
            const auto& inst = bytecode.code[i];
            if (inst.opcode == OpCode::JUMP || inst.opcode == OpCode::JUMP_IF_FALSE || inst.opcode == OpCode::JUMP_IF_TRUE) {
                std::size_t target = static_cast<std::size_t>(static_cast<long long>(i + 1) + inst.offset);
                if (target > bytecode.code.size()) {
                    std::cerr << "[FAIL] jump offset out of range at " << i << std::endl;
                    return 1;
                }
            }
        }

        std::cout << "Codegen smoke test passed." << std::endl;
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Codegen smoke test failed: " << ex.what() << std::endl;
        return 1;
    }
}
