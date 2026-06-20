// Unit tests for MacroExpander: build AST nodes by hand, expand, verify result.
// Compilable with: g++ -std=c++17 -Iinclude tests/macro_expander_test.cpp src/macro_expander.cpp

#include <cassert>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

#include "ast.hpp"
#include "macro_expander.hpp"

// ---- helpers ----

static std::unique_ptr<Expr> num(double v) {
    return std::make_unique<NumberLiteral>(v, 1);
}
static std::unique_ptr<Expr> str(const std::string& v) {
    return std::make_unique<StringLiteral>(v, 1);
}
static std::unique_ptr<Expr> var(const std::string& n) {
    return std::make_unique<VarRef>(n, 1);
}

// Build a minimal Program with macros and a global expression.
static Program make_program(
        std::vector<std::unique_ptr<MacroDef>> macros,
        std::unique_ptr<Expr> global) {
    std::vector<std::unique_ptr<TypeDef>> types;
    std::vector<std::unique_ptr<ProtocolDef>> protos;
    std::vector<std::unique_ptr<FuncDef>> funcs;
    return Program(std::move(types), std::move(protos), std::move(funcs),
                   std::move(macros), std::move(global), 1);
}

// ---- tests ----

// 1. Simple value-param substitution.
//    def double(x: Number): Number => x + x;
//    double(5)  =>  5 + 5
static void test_simple_substitution() {
    // Build macro def: body = x + x
    auto body = std::make_unique<BinaryExpr>("+", var("x"), var("x"), 1);
    std::vector<MacroParam> params;
    params.emplace_back("x", "Number");
    auto macro_def = std::make_unique<MacroDef>("double", std::move(params), "Number", std::move(body), 1);

    // Build invocation: double(5)
    std::vector<std::unique_ptr<Expr>> args;
    args.push_back(num(5.0));
    auto invoke = std::make_unique<MacroInvoke>("double", std::move(args), nullptr, 1);

    std::vector<std::unique_ptr<MacroDef>> macros;
    macros.push_back(std::move(macro_def));
    auto program = make_program(std::move(macros), std::move(invoke));

    MacroExpander expander;
    expander.expand(program);

    // After expansion: global_expression should be BinaryExpr(+, 5, 5)
    auto* bin = dynamic_cast<BinaryExpr*>(program.global_expression.get());
    assert(bin && "expected BinaryExpr after expansion");
    assert(bin->op == "+");
    auto* lhs = dynamic_cast<NumberLiteral*>(bin->left.get());
    auto* rhs = dynamic_cast<NumberLiteral*>(bin->right.get());
    assert(lhs && lhs->value == 5.0);
    assert(rhs && rhs->value == 5.0);
    std::cout << "[OK] simple substitution: double(5) => 5+5\n";
}

// 2. Syntactic (*expr) parameter substitution.
//    def twice(*body: Object): Object => { body; body; }
//    twice() { 42 }  =>  { 42; 42 }
static void test_syntactic_param() {
    // body_expr = block { body; body; }
    std::vector<std::unique_ptr<Expr>> stmts;
    stmts.push_back(var("body"));
    stmts.push_back(var("body"));
    auto body = std::make_unique<BlockExpr>(std::move(stmts), 1);

    std::vector<MacroParam> params;
    params.emplace_back("body", "Object", /*is_syntactic=*/true);
    auto macro_def = std::make_unique<MacroDef>("twice", std::move(params), "Object", std::move(body), 1);

    // Invocation: twice() { 42 }
    auto block_arg = std::make_unique<NumberLiteral>(42.0, 1);
    std::vector<std::unique_ptr<Expr>> args;  // no value args
    auto invoke = std::make_unique<MacroInvoke>("twice", std::move(args), std::move(block_arg), 1);

    std::vector<std::unique_ptr<MacroDef>> macros;
    macros.push_back(std::move(macro_def));
    auto program = make_program(std::move(macros), std::move(invoke));

    MacroExpander expander;
    expander.expand(program);

    // Result: BlockExpr with 2 NumberLiteral(42) children.
    auto* blk = dynamic_cast<BlockExpr*>(program.global_expression.get());
    assert(blk && "expected BlockExpr");
    assert(blk->expressions.size() == 2);
    auto* e0 = dynamic_cast<NumberLiteral*>(blk->expressions[0].get());
    auto* e1 = dynamic_cast<NumberLiteral*>(blk->expressions[1].get());
    assert(e0 && e0->value == 42.0);
    assert(e1 && e1->value == 42.0);
    std::cout << "[OK] syntactic param: twice(){42} => {42;42}\n";
}

// 3. Mixed params: one value + one syntactic.
//    def repeat_n(n: Number, *expr: Object): Object =>
//        let i = n in while (i > 0) { i := i - 1; expr; };
//    repeat_n(3) { print(1) }
//    => let i = 3 in while (i > 0) { i := i - 1; print(1); }
static void test_mixed_params() {
    // Build body: let i = n in while (i > 0) { i := i-1; expr; }
    auto decrement = std::make_unique<BinaryExpr>("-", var("i"), num(1.0), 1);
    auto assign_i  = std::make_unique<AssignExpr>("i", std::move(decrement), 1);
    std::vector<std::unique_ptr<Expr>> block_stmts;
    block_stmts.push_back(std::move(assign_i));
    block_stmts.push_back(var("expr"));
    auto loop_body = std::make_unique<BlockExpr>(std::move(block_stmts), 1);

    auto cond = std::make_unique<BinaryExpr>(">", var("i"), num(0.0), 1);
    auto while_loop = std::make_unique<WhileExpr>(std::move(cond), std::move(loop_body), 1);

    std::vector<LetBinding> binds;
    binds.emplace_back("i", "", var("n"));
    auto body = std::make_unique<LetExpr>(std::move(binds), std::move(while_loop), 1);

    std::vector<MacroParam> params;
    params.emplace_back("n", "Number");
    params.emplace_back("expr", "Object", /*syntactic=*/true);
    auto macro_def = std::make_unique<MacroDef>("repeat_n", std::move(params), "Object", std::move(body), 1);

    // Invocation: repeat_n(3) { print(1) }
    std::vector<std::unique_ptr<Expr>> args;
    args.push_back(num(3.0));
    std::vector<std::unique_ptr<Expr>> print_args;
    print_args.push_back(num(1.0));
    auto block_arg = std::make_unique<FuncCall>("print", std::move(print_args), 1);
    auto invoke = std::make_unique<MacroInvoke>("repeat_n", std::move(args), std::move(block_arg), 1);

    std::vector<std::unique_ptr<MacroDef>> macros;
    macros.push_back(std::move(macro_def));
    auto program = make_program(std::move(macros), std::move(invoke));

    MacroExpander expander;
    expander.expand(program);

    // Result: LetExpr binding i=3
    auto* let = dynamic_cast<LetExpr*>(program.global_expression.get());
    assert(let && "expected LetExpr");
    assert(let->bindings.size() == 1 && let->bindings[0].name == "i");
    auto* n3 = dynamic_cast<NumberLiteral*>(let->bindings[0].initializer.get());
    assert(n3 && n3->value == 3.0);
    // Body is a WhileExpr
    auto* wh = dynamic_cast<WhileExpr*>(let->body.get());
    assert(wh && "expected WhileExpr inside let");
    // Body of while is a block with 2 stmts; second stmt is FuncCall("print")
    auto* blk = dynamic_cast<BlockExpr*>(wh->body.get());
    assert(blk && blk->expressions.size() == 2);
    auto* call = dynamic_cast<FuncCall*>(blk->expressions[1].get());
    assert(call && call->name == "print");
    std::cout << "[OK] mixed params: repeat_n(3){print(1)} expanded correctly\n";
}

// 4. match/case desugar.
//    match (x) { case Number: 1; case String: 2; default: 3; }
//    =>  let __match0__ = x in if (__match0__ is Number) 1 elif (...) 2 else 3
static void test_match_desugar() {
    std::vector<MatchArm> arms;
    arms.emplace_back("Number", num(1.0), 1);
    arms.emplace_back("String", num(2.0), 1);
    auto match = std::make_unique<MatchExpr>(var("x"), std::move(arms), num(3.0), 1);

    std::vector<std::unique_ptr<MacroDef>> macros;
    auto program = make_program(std::move(macros), std::move(match));

    MacroExpander expander;
    expander.expand(program);

    // Result: LetExpr(binding=__match..., body=IfExpr)
    auto* let = dynamic_cast<LetExpr*>(program.global_expression.get());
    assert(let && "expected LetExpr from match desugar");
    assert(let->bindings.size() == 1);
    // Binding initializer is VarRef("x")
    auto* vref = dynamic_cast<VarRef*>(let->bindings[0].initializer.get());
    assert(vref && vref->name == "x");
    // Body is IfExpr with 2 branches + else
    auto* iexpr = dynamic_cast<IfExpr*>(let->body.get());
    assert(iexpr && iexpr->branches.size() == 2);
    // First branch condition: __match__ is Number
    auto* is0 = dynamic_cast<IsExpr*>(iexpr->branches[0].condition.get());
    assert(is0 && is0->type_name == "Number");
    auto* is1 = dynamic_cast<IsExpr*>(iexpr->branches[1].condition.get());
    assert(is1 && is1->type_name == "String");
    // else body: 3
    auto* def = dynamic_cast<NumberLiteral*>(iexpr->else_body.get());
    assert(def && def->value == 3.0);
    std::cout << "[OK] match desugar: 2 arms + default\n";
}

// 5. Undefined macro → error.
static void test_undefined_macro() {
    std::vector<std::unique_ptr<Expr>> args;
    auto invoke = std::make_unique<MacroInvoke>("nonexistent", std::move(args), nullptr, 1);
    std::vector<std::unique_ptr<MacroDef>> macros;
    auto program = make_program(std::move(macros), std::move(invoke));
    MacroExpander expander;
    try {
        expander.expand(program);
        assert(false && "should have thrown");
    } catch (const std::runtime_error& e) {
        std::cout << "[OK] undefined macro raises error: " << e.what() << "\n";
    }
}

// 6. Wrong arity → error.
static void test_arity_mismatch() {
    auto body = std::make_unique<NumberLiteral>(0.0, 1);
    std::vector<MacroParam> params;
    params.emplace_back("x", "Number");
    auto macro_def = std::make_unique<MacroDef>("one_param", std::move(params), "Number", std::move(body), 1);

    // Call with 2 args instead of 1.
    std::vector<std::unique_ptr<Expr>> args;
    args.push_back(num(1.0));
    args.push_back(num(2.0));
    auto invoke = std::make_unique<MacroInvoke>("one_param", std::move(args), nullptr, 1);

    std::vector<std::unique_ptr<MacroDef>> macros;
    macros.push_back(std::move(macro_def));
    auto program = make_program(std::move(macros), std::move(invoke));
    MacroExpander expander;
    try {
        expander.expand(program);
        assert(false && "should have thrown");
    } catch (const std::runtime_error& e) {
        std::cout << "[OK] arity mismatch raises error: " << e.what() << "\n";
    }
}

// 7. Nested macro: macro using another macro.
//    def addone(x: Number) => x + 1;
//    def addtwo(x: Number) => addone(addone(x));
//    addtwo(5)  =>  (5+1)+1 = 7 (structurally)
static void test_nested_macro() {
    // addone: x + 1
    auto body1 = std::make_unique<BinaryExpr>("+", var("x"), num(1.0), 1);
    std::vector<MacroParam> p1; p1.emplace_back("x", "Number");
    auto addone = std::make_unique<MacroDef>("addone", std::move(p1), "Number", std::move(body1), 1);

    // addtwo: addone(addone(x))
    std::vector<std::unique_ptr<Expr>> inner_args; inner_args.push_back(var("x"));
    auto inner_inv = std::make_unique<MacroInvoke>("addone", std::move(inner_args), nullptr, 1);
    std::vector<std::unique_ptr<Expr>> outer_args; outer_args.push_back(std::move(inner_inv));
    auto body2 = std::make_unique<MacroInvoke>("addone", std::move(outer_args), nullptr, 1);
    std::vector<MacroParam> p2; p2.emplace_back("x", "Number");
    auto addtwo = std::make_unique<MacroDef>("addtwo", std::move(p2), "Number", std::move(body2), 1);

    // Invoke addtwo(5)
    std::vector<std::unique_ptr<Expr>> call_args; call_args.push_back(num(5.0));
    auto invoke = std::make_unique<MacroInvoke>("addtwo", std::move(call_args), nullptr, 1);

    std::vector<std::unique_ptr<MacroDef>> macros;
    macros.push_back(std::move(addone));
    macros.push_back(std::move(addtwo));
    auto program = make_program(std::move(macros), std::move(invoke));

    MacroExpander expander;
    expander.expand(program);

    // Result: (5+1)+1 → BinaryExpr(+, BinaryExpr(+,5,1), 1)
    auto* outer = dynamic_cast<BinaryExpr*>(program.global_expression.get());
    assert(outer && outer->op == "+");
    auto* inner = dynamic_cast<BinaryExpr*>(outer->left.get());
    assert(inner && inner->op == "+");
    auto* five = dynamic_cast<NumberLiteral*>(inner->left.get());
    assert(five && five->value == 5.0);
    std::cout << "[OK] nested macros: addtwo(5) => (5+1)+1\n";
}

int main() {
    std::cout << "=== MacroExpander unit tests ===\n";
    try {
        test_simple_substitution();
        test_syntactic_param();
        test_mixed_params();
        test_match_desugar();
        test_undefined_macro();
        test_arity_mismatch();
        test_nested_macro();
        std::cout << "All macro_expander tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "FAIL: " << ex.what() << "\n";
        return 1;
    }
}
