#include <cassert>
#include <iostream>
#include <stdexcept>

#include "ast.hpp"
#include "semantic_analyzer.hpp"

// Construye un Program mínimo con solo una expresión global
static Program make_program(std::unique_ptr<Expr> global) {
    return Program({}, {}, {}, std::move(global), 1);
}

// Helper para construir un Program con una función global + expresión
static Program make_program_with_func(std::unique_ptr<FuncDef> func,
                                      std::unique_ptr<Expr> global) {
    std::vector<std::unique_ptr<FuncDef>> funcs;
    funcs.push_back(std::move(func));
    return Program({}, {}, std::move(funcs), std::move(global), 1);
}

// Verifica que analizar `prog` NO lanza excepción
static void assert_ok(Program& prog, const std::string& test_name) {
    try {
        SemanticAnalyzer analyzer;
        analyzer.analyze(prog);
        std::cout << "[PASS] " << test_name << "\n";
    } catch (const std::exception& e) {
        std::cerr << "[FAIL] " << test_name << ": " << e.what() << "\n";
        std::exit(1);
    }
}

// Verifica que analizar `prog` SÍ lanza excepción (caso de error)
static void assert_error(Program& prog, const std::string& test_name) {
    try {
        SemanticAnalyzer analyzer;
        analyzer.analyze(prog);
        std::cerr << "[FAIL] " << test_name << ": se esperaba error semántico\n";
        std::exit(1);
    } catch (const std::exception&) {
        std::cout << "[PASS] " << test_name << " (error esperado)\n";
    }
}

// ─── Test 1: lambda sin parámetros ─────────────────────────────────────────
// () => 42
static void test_lambda_no_params() {
    auto body = std::make_unique<NumberLiteral>(42.0, 1);
    auto lambda = std::make_unique<LambdaExpr>(
        std::vector<Parameter>{}, "", std::move(body), 1);
    auto prog = make_program(std::move(lambda));
    assert_ok(prog, "lambda sin parámetros: () => 42");
}

// ─── Test 2: lambda con un parámetro tipado ─────────────────────────────────
// (x: Number): Number => x * 2
static void test_lambda_one_param() {
    std::vector<Parameter> params;
    params.emplace_back("x", "Number");

    auto body = std::make_unique<BinaryExpr>(
        "*",
        std::make_unique<VarRef>("x", 1),
        std::make_unique<NumberLiteral>(2.0, 1),
        1);
    auto lambda = std::make_unique<LambdaExpr>(
        std::move(params), "Number", std::move(body), 1);
    auto prog = make_program(std::move(lambda));
    assert_ok(prog, "lambda un parámetro: (x: Number): Number => x * 2");
}

// ─── Test 3: lambda con dos parámetros ──────────────────────────────────────
// (x: Number, y: Number): Number => x + y
static void test_lambda_two_params() {
    std::vector<Parameter> params;
    params.emplace_back("x", "Number");
    params.emplace_back("y", "Number");

    auto body = std::make_unique<BinaryExpr>(
        "+",
        std::make_unique<VarRef>("x", 1),
        std::make_unique<VarRef>("y", 1),
        1);
    auto lambda = std::make_unique<LambdaExpr>(
        std::move(params), "Number", std::move(body), 1);
    auto prog = make_program(std::move(lambda));
    assert_ok(prog, "lambda dos parámetros: (x: Number, y: Number): Number => x + y");
}

// ─── Test 4: lambda retorna tipo correcto (Boolean) ─────────────────────────
// (x: Number): Boolean => x > 0
static void test_lambda_returns_bool() {
    std::vector<Parameter> params;
    params.emplace_back("x", "Number");

    auto body = std::make_unique<BinaryExpr>(
        ">",
        std::make_unique<VarRef>("x", 1),
        std::make_unique<NumberLiteral>(0.0, 1),
        1);
    auto lambda = std::make_unique<LambdaExpr>(
        std::move(params), "Boolean", std::move(body), 1);
    auto prog = make_program(std::move(lambda));
    assert_ok(prog, "lambda retorna Boolean: (x: Number): Boolean => x > 0");
}

// ─── Test 5: lambda generada queda registrada como tipo ─────────────────────
// Verificamos que después del análisis, generated_type_name está relleno.
static void test_lambda_type_registered() {
    std::vector<Parameter> params;
    params.emplace_back("x", "Number");

    auto body = std::make_unique<BinaryExpr>(
        "*",
        std::make_unique<VarRef>("x", 1),
        std::make_unique<NumberLiteral>(3.0, 1),
        1);
    auto* lambda_raw = new LambdaExpr(std::move(params), "Number", std::move(body), 1);
    auto lambda = std::unique_ptr<LambdaExpr>(lambda_raw);

    auto prog = make_program(std::move(lambda));
    SemanticAnalyzer analyzer;
    analyzer.analyze(prog);

    assert(!lambda_raw->generated_type_name.empty() &&
           lambda_raw->generated_type_name.rfind("_Lambda_", 0) == 0);
    std::cout << "[PASS] lambda registra tipo: generated_type_name = '"
              << lambda_raw->generated_type_name << "'\n";
}

// ─── Test 6: llamada a functor — f(x) donde f es una lambda ─────────────────
// let f = (x: Number): Number => x * 2 in f(5)
static void test_functor_call() {
    // La lambda: (x: Number): Number => x * 2
    std::vector<Parameter> params;
    params.emplace_back("x", "Number");
    auto lambda_body = std::make_unique<BinaryExpr>(
        "*",
        std::make_unique<VarRef>("x", 1),
        std::make_unique<NumberLiteral>(2.0, 1),
        1);
    auto lambda = std::make_unique<LambdaExpr>(
        std::move(params), "Number", std::move(lambda_body), 1);

    // La llamada: f(5)
    std::vector<std::unique_ptr<Expr>> call_args;
    call_args.push_back(std::make_unique<NumberLiteral>(5.0, 1));
    auto* call_raw = new FuncCall("f", std::move(call_args), 1);
    auto call = std::unique_ptr<FuncCall>(call_raw);

    // let f = lambda in f(5)
    std::vector<LetBinding> bindings;
    bindings.emplace_back("f", "", std::move(lambda));
    auto let_expr = std::make_unique<LetExpr>(std::move(bindings), std::move(call), 1);

    auto prog = make_program(std::move(let_expr));
    SemanticAnalyzer analyzer;
    analyzer.analyze(prog);

    assert(call_raw->is_functor == true);
    std::cout << "[PASS] functor call: let f = (x: Number) => x * 2 in f(5) → is_functor=true\n";
}

// ─── Test 7: lambda con captura de variable libre ───────────────────────────
// let threshold = 5 in let filter = (x: Number): Boolean => x > threshold in filter(10)
static void test_lambda_captures_var() {
    // threshold = 5
    auto threshold_val = std::make_unique<NumberLiteral>(5.0, 1);

    // filter = (x: Number): Boolean => x > threshold
    std::vector<Parameter> params;
    params.emplace_back("x", "Number");
    auto lambda_body = std::make_unique<BinaryExpr>(
        ">",
        std::make_unique<VarRef>("x", 1),
        std::make_unique<VarRef>("threshold", 1),
        1);
    auto* lambda_raw = new LambdaExpr(
        std::move(params), "Boolean", std::move(lambda_body), 1);

    // filter(10)
    std::vector<std::unique_ptr<Expr>> call_args;
    call_args.push_back(std::make_unique<NumberLiteral>(10.0, 1));
    auto call = std::make_unique<FuncCall>("filter", std::move(call_args), 1);

    // let filter = lambda in filter(10)
    std::vector<LetBinding> inner_bindings;
    inner_bindings.emplace_back("filter", "", std::unique_ptr<LambdaExpr>(lambda_raw));
    auto inner = std::make_unique<LetExpr>(std::move(inner_bindings), std::move(call), 1);

    // let threshold = 5 in <inner>
    std::vector<LetBinding> outer_bindings;
    outer_bindings.emplace_back("threshold", "Number", std::move(threshold_val));
    auto outer = std::make_unique<LetExpr>(std::move(outer_bindings), std::move(inner), 1);

    auto prog = make_program(std::move(outer));
    SemanticAnalyzer analyzer;
    analyzer.analyze(prog);

    assert(!lambda_raw->captured_vars.empty());
    assert(lambda_raw->captured_vars[0] == "threshold");
    std::cout << "[PASS] lambda captura variable: captured_vars = ['"
              << lambda_raw->captured_vars[0] << "']\n";
}

// ─── Test 8: dos lambdas reciben nombres únicos ─────────────────────────────
static void test_two_lambdas_unique_names() {
    auto body1 = std::make_unique<NumberLiteral>(1.0, 1);
    auto body2 = std::make_unique<NumberLiteral>(2.0, 1);
    auto* l1 = new LambdaExpr({}, "", std::move(body1), 1);
    auto* l2 = new LambdaExpr({}, "", std::move(body2), 1);

    // { l1; l2 } como bloque
    std::vector<std::unique_ptr<Expr>> exprs;
    exprs.push_back(std::unique_ptr<LambdaExpr>(l1));
    exprs.push_back(std::unique_ptr<LambdaExpr>(l2));
    auto block = std::make_unique<BlockExpr>(std::move(exprs), 1);

    auto prog = make_program(std::move(block));
    SemanticAnalyzer analyzer;
    analyzer.analyze(prog);

    assert(l1->generated_type_name != l2->generated_type_name);
    std::cout << "[PASS] dos lambdas tienen nombres únicos: '"
              << l1->generated_type_name << "' vs '"
              << l2->generated_type_name << "'\n";
}

// ─── Test 9 (negativo): lambda usada sin invoke donde no corresponde ─────────
// Llamar f(5) cuando f no tiene invoke — debe dar error
static void test_functor_call_on_non_functor() {
    // let f = 42 in f(5)  — f es Number, no tiene invoke
    std::vector<std::unique_ptr<Expr>> call_args;
    call_args.push_back(std::make_unique<NumberLiteral>(5.0, 1));
    auto call = std::make_unique<FuncCall>("f", std::move(call_args), 1);

    std::vector<LetBinding> bindings;
    bindings.emplace_back("f", "Number", std::make_unique<NumberLiteral>(42.0, 1));
    auto let_expr = std::make_unique<LetExpr>(std::move(bindings), std::move(call), 1);

    auto prog = make_program(std::move(let_expr));
    assert_error(prog, "llamar f(5) cuando f es Number (no functor)");
}

int main() {
    std::cout << "=== Lambda Smoke Tests ===\n";
    test_lambda_no_params();
    test_lambda_one_param();
    test_lambda_two_params();
    test_lambda_returns_bool();
    test_lambda_type_registered();
    test_functor_call();
    test_lambda_captures_var();
    test_two_lambdas_unique_names();
    test_functor_call_on_non_functor();
    std::cout << "=== Todos los tests pasaron ===\n";
    return 0;
}
