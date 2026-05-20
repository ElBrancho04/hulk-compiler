#include <cassert>
#include <iostream>
#include <stdexcept>

#include "semantic_analyzer.hpp"

// Construye un Program mínimo con una función y una expresión global.
static Program make_program(std::vector<std::unique_ptr<FuncDef>> funcs,
                             std::unique_ptr<Expr> global) {
    return Program(
        {},
        {},
        std::move(funcs),
        std::move(global),
        1);
}

// Verifica que analyze() no lanza excepción.
static void assert_ok(Program& p, const char* label) {
    SemanticAnalyzer analyzer;
    try {
        analyzer.analyze(p);
        std::cout << "  OK: " << label << "\n";
    } catch (const std::exception& e) {
        std::cerr << "  FAIL: " << label << " -> " << e.what() << "\n";
        std::exit(1);
    }
}

// Verifica que analyze() SÍ lanza excepción (caso inválido).
static void assert_error(Program& p, const char* label) {
    SemanticAnalyzer analyzer;
    try {
        analyzer.analyze(p);
        std::cerr << "  FAIL (expected error): " << label << "\n";
        std::exit(1);
    } catch (const std::exception&) {
        std::cout << "  OK (error expected): " << label << "\n";
    }
}

int main() {
    std::cout << "=== type_annotations_smoke_test ===\n";

    // ----------------------------------------------------------------
    // 1. Función con parámetro anotado Iterable<Number> (sintaxis T*)
    //    El parser produce "Iterable<Number>" como string; lo simulamos
    //    directamente en el AST.
    // ----------------------------------------------------------------
    {
        // function sum(numbers: Iterable<Number>): Number => 0;
        std::vector<Parameter> params;
        params.emplace_back("numbers", "Iterable<Number>");
        auto func = std::make_unique<FuncDef>(
            "sum", params, "Number",
            std::make_unique<NumberLiteral>(0.0, 1), 1);

        std::vector<std::unique_ptr<FuncDef>> funcs;
        funcs.push_back(std::move(func));
        auto prog = make_program(std::move(funcs),
                                 std::make_unique<NumberLiteral>(0.0, 1));
        assert_ok(prog, "funcion con parametro Iterable<Number>");
    }

    // ----------------------------------------------------------------
    // 2. Función con parámetro anotado Vector<Number> (sintaxis T[])
    // ----------------------------------------------------------------
    {
        // function mean(arr: Vector<Number>): Number => 0;
        std::vector<Parameter> params;
        params.emplace_back("arr", "Vector<Number>");
        auto func = std::make_unique<FuncDef>(
            "mean", params, "Number",
            std::make_unique<NumberLiteral>(0.0, 1), 1);

        std::vector<std::unique_ptr<FuncDef>> funcs;
        funcs.push_back(std::move(func));
        auto prog = make_program(std::move(funcs),
                                 std::make_unique<NumberLiteral>(0.0, 1));
        assert_ok(prog, "funcion con parametro Vector<Number>");
    }

    // ----------------------------------------------------------------
    // 3. Función con return type Iterable<Number>
    // ----------------------------------------------------------------
    {
        // function nums(): Iterable<Number> => range(0, 10);
        // Simplificamos el body a un literal (el tipo no se verifica aquí)
        std::vector<Parameter> params;
        auto func = std::make_unique<FuncDef>(
            "nums", params, "Iterable<Number>",
            std::make_unique<FuncCall>(
                "range",
                []{
                    std::vector<std::unique_ptr<Expr>> args;
                    args.push_back(std::make_unique<NumberLiteral>(0.0, 1));
                    args.push_back(std::make_unique<NumberLiteral>(10.0, 1));
                    return args;
                }(),
                1),
            1);

        std::vector<std::unique_ptr<FuncDef>> funcs;
        funcs.push_back(std::move(func));
        auto prog = make_program(std::move(funcs),
                                 std::make_unique<NumberLiteral>(0.0, 1));
        assert_ok(prog, "funcion con return type Iterable<Number>");
    }

    // ----------------------------------------------------------------
    // 4. Vector<Number> conforma a Iterable<Number>:
    //    let x: Iterable<Number> = range(0, 10)
    //    range() devuelve Iterable<Number>, que conforma a Iterable<Number>
    // ----------------------------------------------------------------
    {
        std::vector<std::unique_ptr<Expr>> range_args;
        range_args.push_back(std::make_unique<NumberLiteral>(0.0, 1));
        range_args.push_back(std::make_unique<NumberLiteral>(5.0, 1));

        std::vector<LetBinding> bindings;
        bindings.emplace_back("x", "Iterable<Number>",
                              std::make_unique<FuncCall>("range", std::move(range_args), 1));

        auto global = std::make_unique<LetExpr>(
            std::move(bindings),
            std::make_unique<NumberLiteral>(0.0, 1),
            1);

        auto prog = make_program({}, std::move(global));
        assert_ok(prog, "let x: Iterable<Number> = range(0,5)");
    }

    // ----------------------------------------------------------------
    // 5. Tipo desconocido en anotación debe dar error semántico
    //    function bad(x: Iterable<Phantom>): Number => 0;
    // ----------------------------------------------------------------
    {
        std::vector<Parameter> params;
        params.emplace_back("x", "Iterable<Phantom>");
        auto func = std::make_unique<FuncDef>(
            "bad", params, "Number",
            std::make_unique<NumberLiteral>(0.0, 1), 1);

        std::vector<std::unique_ptr<FuncDef>> funcs;
        funcs.push_back(std::move(func));
        auto prog = make_program(std::move(funcs),
                                 std::make_unique<NumberLiteral>(0.0, 1));
        assert_error(prog, "Iterable<Phantom> debe fallar (tipo inexistente)");
    }

    std::cout << "type_annotations_smoke_test: todos los tests pasaron.\n";
    return 0;
}
