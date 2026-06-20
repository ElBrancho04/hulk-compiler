#include <cassert>
#include <iostream>
#include <stdexcept>
#include <sstream>

#include "ast.hpp"
#include "semantic_analyzer.hpp"
#include "code_generator.hpp"
#include "vm.hpp"
#include "value.hpp"

// Test 1: Functor call con lambda — f(x) donde f es lambda
// let f = (x: Number): Number => x * 2 in f(5)
static void test_lambda_functor_call() {
    std::vector<Parameter> params;
    params.emplace_back("x", "Number");
    auto lambda_body = std::make_unique<BinaryExpr>(
        "*",
        std::make_unique<VarRef>("x", 1),
        std::make_unique<NumberLiteral>(2.0, 1),
        1);
    auto lambda = std::make_unique<LambdaExpr>(
        std::move(params), "Number", std::move(lambda_body), 1);

    std::vector<std::unique_ptr<Expr>> call_args;
    call_args.push_back(std::make_unique<NumberLiteral>(5.0, 1));
    auto call = std::make_unique<FuncCall>("f", std::move(call_args), 1);

    std::vector<LetBinding> bindings;
    bindings.emplace_back("f", "", std::move(lambda));
    auto let_expr = std::make_unique<LetExpr>(std::move(bindings), std::move(call), 1);

    Program prog(
        std::vector<std::unique_ptr<TypeDef>>(),
        std::vector<std::unique_ptr<ProtocolDef>>(),
        std::vector<std::unique_ptr<FuncDef>>(),
        std::move(let_expr),
        1
    );

    try {
        SemanticAnalyzer analyzer;
        analyzer.analyze(prog);

        BytecodeProgram bytecode;
        CodeGenerator cg(bytecode);
        prog.accept(cg);

        VM vm;
        vm.execute(bytecode);

        // The result should be 10 on the stack
        Value result = vm.peekStack();
        assert(result.type == ValueType::Number);
        assert(result.number_value == 10.0);
        std::cout << "[PASS] test_lambda_functor_call: result = " << result.number_value << " (expected 10)\n";
    } catch (const std::exception& e) {
        std::cerr << "[FAIL] test_lambda_functor_call: " << e.what() << "\n";
        std::exit(1);
    }
}

// Test 2: Pasar función global como functor donde se espera un protocolo functor
// function apply(f: NumberFilter, x: Number): Number => f(x);
// function is_even(x: Number): Boolean => x % 2 == 0;
// apply(is_even, 4)  // should be true
static void test_function_wrapper() {
    // Crear función apply(f: NumberFilter, x: Number): Number => f(x)
    std::vector<Parameter> apply_params;
    apply_params.emplace_back("f", "NumberFilter");
    apply_params.emplace_back("x", "Number");

    auto apply_body = std::make_unique<FuncCall>(
        "f",
        std::vector<std::unique_ptr<Expr>>(),
        1);
    apply_body->args.push_back(std::make_unique<VarRef>("x", 1));

    // Declaracion de funcion apply: se agrega a func_defs mas abajo.
    auto apply_def = std::make_unique<FuncDef>(
        "apply", apply_params, "Boolean", std::move(apply_body), 1);

    // Protocolo NumberFilter with invoke(x: Number): Boolean
    std::vector<std::unique_ptr<ProtocolMethodSig>> proto_methods;
    std::vector<Parameter> invoke_params;
    invoke_params.emplace_back("x", "Number");
    proto_methods.push_back(std::make_unique<ProtocolMethodSig>(
        "invoke", invoke_params, "Boolean", 1));

    std::vector<std::unique_ptr<ProtocolDef>> protocols;
    protocols.push_back(std::make_unique<ProtocolDef>(
        "NumberFilter", "",
        std::move(proto_methods), 1));

    // Función is_even(x: Number): Boolean
    std::vector<Parameter> func_params;
    func_params.emplace_back("x", "Number");
    auto func_body = std::make_unique<BinaryExpr>(
        "==",
        std::make_unique<BinaryExpr>(
            "%",
            std::make_unique<VarRef>("x", 1),
            std::make_unique<NumberLiteral>(2.0, 1),
            1),
        std::make_unique<NumberLiteral>(0.0, 1),
        1);

    std::vector<std::unique_ptr<FuncDef>> func_defs;
    func_defs.push_back(std::move(apply_def));
    func_defs.push_back(std::make_unique<FuncDef>(
        "is_even", func_params, "Boolean", std::move(func_body), 1));

    // Llamada: apply(is_even, 4)
    std::vector<std::unique_ptr<Expr>> call_args;
    call_args.push_back(std::make_unique<VarRef>("is_even", 1));
    call_args.push_back(std::make_unique<NumberLiteral>(4.0, 1));
    auto global = std::make_unique<FuncCall>("apply", std::move(call_args), 1);

    Program prog(
        std::vector<std::unique_ptr<TypeDef>>(),
        std::move(protocols),
        std::move(func_defs),
        std::move(global),
        1
    );

    try {
        SemanticAnalyzer analyzer;
        analyzer.analyze(prog);

        BytecodeProgram bytecode;
        CodeGenerator cg(bytecode);
        prog.accept(cg);

        VM vm;
        vm.execute(bytecode);

        Value result = vm.peekStack();
        assert(result.type == ValueType::Boolean);
        assert(result.bool_value == true);
        std::cout << "[PASS] test_function_wrapper: result = " 
                  << (result.bool_value ? "true" : "false") << " (expected true)\n";
    } catch (const std::exception& e) {
        std::cerr << "[FAIL] test_function_wrapper: " << e.what() << "\n";
        std::exit(1);
    }
}

// Test 3: Functor type annotation parsing
static void test_functional_type_annotation() {
    std::vector<std::string> param_types;
    std::string return_type;

    // Parse _FuncType(Number)->Boolean
    bool result = TypeTable::parse_functional_annotation(
        "_FuncType(Number)->Boolean",
        &param_types,
        &return_type);

    assert(result);
    assert(param_types.size() == 1);
    assert(param_types[0] == "Number");
    assert(return_type == "Boolean");

    // Parse _FuncType(Number,Number)->Number
    param_types.clear();
    return_type.clear();
    result = TypeTable::parse_functional_annotation(
        "_FuncType(Number,Number)->Number",
        &param_types,
        &return_type);

    assert(result);
    assert(param_types.size() == 2);
    assert(param_types[0] == "Number");
    assert(param_types[1] == "Number");
    assert(return_type == "Number");

    // Parse _FuncType()->Object
    param_types.clear();
    return_type.clear();
    result = TypeTable::parse_functional_annotation(
        "_FuncType()->Object",
        &param_types,
        &return_type);

    assert(result);
    assert(param_types.empty());
    assert(return_type == "Object");

    std::cout << "[PASS] test_functional_type_annotation: parseo correcto\n";
}

// Test 4: Functor type registration
static void test_functor_type_registration() {
    TypeTable tt;

    tt.ensure_functor_type({"Number"}, "Boolean");
    std::string functor_name = tt.make_functor_type_name({"Number"}, "Boolean");
    assert(tt.has_type(functor_name));
    assert(tt.has_protocol(functor_name));

    const TypeInfo& info = tt.get_type(functor_name);
    assert(info.methods.count("invoke") > 0);
    const MethodSig& invoke_sig = info.methods.at("invoke");
    assert(invoke_sig.param_types.size() == 1);
    assert(invoke_sig.param_types[0] == "Number");
    assert(invoke_sig.return_type == "Boolean");

    std::string functor_name2 = tt.make_functor_type_name({"Number"}, "Boolean");
    assert(functor_name == functor_name2);

    std::cout << "[PASS] test_functor_type_registration: tipo functor registrado y verificado\n";
}

int main() {
    std::cout << "=== Functor Smoke Tests ===\n";
    test_functional_type_annotation();
    test_functor_type_registration();
    test_lambda_functor_call();
    test_function_wrapper();
    std::cout << "=== Todos los tests pasaron ===\n";
    return 0;
}