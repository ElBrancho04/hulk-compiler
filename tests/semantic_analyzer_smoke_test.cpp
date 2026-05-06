#include <iostream>

#include "semantic_analyzer.hpp"

int main() {
    std::vector<std::unique_ptr<AttributeDef>> attrs;
    attrs.push_back(std::make_unique<AttributeDef>(
        "x",
        "Number",
        std::make_unique<NumberLiteral>(1.0, 1),
        1));

    std::vector<Parameter> method_params;
    std::vector<std::unique_ptr<MethodDef>> methods;
    methods.push_back(std::make_unique<MethodDef>(
        "m",
        method_params,
        "Number",
        std::make_unique<NumberLiteral>(2.0, 2),
        2));

    std::vector<Parameter> type_params;
    std::vector<std::unique_ptr<Expr>> parent_args;
    auto type_def = std::make_unique<TypeDef>(
        "Foo",
        type_params,
        "",
        std::move(parent_args),
        std::move(attrs),
        std::move(methods),
        1);

    std::vector<Parameter> func_params;
    auto func_def = std::make_unique<FuncDef>(
        "f",
        func_params,
        "Number",
        std::make_unique<NumberLiteral>(3.0, 3),
        3);

    std::vector<std::unique_ptr<TypeDef>> types;
    types.push_back(std::move(type_def));

    std::vector<std::unique_ptr<ProtocolDef>> protocols;

    std::vector<std::unique_ptr<FuncDef>> functions;
    functions.push_back(std::move(func_def));

    auto global_expr = std::make_unique<NumberLiteral>(4.0, 4);

    Program program(
        std::move(types),
        std::move(protocols),
        std::move(functions),
        std::move(global_expr),
        1);

    SemanticAnalyzer analyzer;
    analyzer.analyze(program);

    std::cout << "SemanticAnalyzer smoke test passed." << std::endl;
    return 0;
}
