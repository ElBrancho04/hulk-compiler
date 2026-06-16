// End-to-end test: AST -> CodeGenerator -> VM for `for` loops over iterables.
// This is stronger than vm_smoke_test's hand-assembled bytecode: it verifies
// that visit(ForExpr) actually emits the correct pattern AND that the VM runs it.

#include <iostream>
#include <memory>
#include <vector>

#include "ast.hpp"
#include "bytecode.hpp"
#include "code_generator.hpp"
#include "vm.hpp"

namespace {

// Build a Program whose single global expression is `expr`, run it, return result.
Value run_expr(std::unique_ptr<Expr> expr) {
    std::vector<std::unique_ptr<TypeDef>> types;
    std::vector<std::unique_ptr<ProtocolDef>> protocols;
    std::vector<std::unique_ptr<FuncDef>> functions;

    Program program(std::move(types), std::move(protocols),
                    std::move(functions), std::move(expr), 1);

    BytecodeProgram bytecode;
    CodeGenerator generator(bytecode);
    program.accept(generator);

    VM vm;
    vm.execute(bytecode);
    return vm.popStack();
}

std::unique_ptr<Expr> num(double v) {
    return std::make_unique<NumberLiteral>(v, 1);
}

std::unique_ptr<Expr> vec(std::vector<double> values) {
    std::vector<std::unique_ptr<Expr>> elements;
    for (double v : values) {
        elements.push_back(num(v));
    }
    return std::make_unique<VectorLiteral>(std::move(elements), 1);
}

bool fail(const std::string& label, const Value& got) {
    std::cerr << "[FAIL] " << label << " (type=" << static_cast<int>(got.type)
              << " number=" << got.number_value << ")" << std::endl;
    return false;
}

} // namespace

int main() {
    try {
        // 1. for(x in [1,2,3]) x  → last body value = 3
        {
            auto loop = std::make_unique<ForExpr>(
                "x", vec({1.0, 2.0, 3.0}),
                std::make_unique<VarRef>("x", 1), 1);
            Value r = run_expr(std::move(loop));
            if (r.type != ValueType::Number || r.number_value != 3.0) {
                return fail("for over [1,2,3] returns last element", r) ? 1 : 1;
            }
            std::cout << "[OK] for(x in [1,2,3]) x  => 3" << std::endl;
        }

        // 2. for(x in [42]) x  → 42 (single element)
        {
            auto loop = std::make_unique<ForExpr>(
                "x", vec({42.0}),
                std::make_unique<VarRef>("x", 1), 1);
            Value r = run_expr(std::move(loop));
            if (r.type != ValueType::Number || r.number_value != 42.0) {
                return fail("for over single element", r) ? 1 : 1;
            }
            std::cout << "[OK] for(x in [42]) x  => 42" << std::endl;
        }

        // 3. for(x in []) x  → Null (zero iterations: result stays null)
        {
            auto loop = std::make_unique<ForExpr>(
                "x", vec({}),
                std::make_unique<VarRef>("x", 1), 1);
            Value r = run_expr(std::move(loop));
            if (r.type != ValueType::Null) {
                return fail("for over empty vector returns null", r) ? 1 : 1;
            }
            std::cout << "[OK] for(x in []) x  => Null" << std::endl;
        }

        // 4. for(x in range(0,5)) x  → last value 4 (verifies range still works)
        {
            std::vector<std::unique_ptr<Expr>> range_args;
            range_args.push_back(num(0.0));
            range_args.push_back(num(5.0));
            auto range_call = std::make_unique<FuncCall>("range", std::move(range_args), 1);

            auto loop = std::make_unique<ForExpr>(
                "x", std::move(range_call),
                std::make_unique<VarRef>("x", 1), 1);
            Value r = run_expr(std::move(loop));
            if (r.type != ValueType::Number || r.number_value != 4.0) {
                return fail("for over range(0,5) returns last value 4", r) ? 1 : 1;
            }
            std::cout << "[OK] for(x in range(0,5)) x  => 4" << std::endl;
        }

        // 5. Nested for loops: temp vars must not collide
        //    for(i in [1,2]) ( for(j in [10,20,30]) j )  → inner last = 30
        {
            auto inner = std::make_unique<ForExpr>(
                "j", vec({10.0, 20.0, 30.0}),
                std::make_unique<VarRef>("j", 1), 1);
            auto outer = std::make_unique<ForExpr>(
                "i", vec({1.0, 2.0}),
                std::move(inner), 1);
            Value r = run_expr(std::move(outer));
            if (r.type != ValueType::Number || r.number_value != 30.0) {
                return fail("nested for loops", r) ? 1 : 1;
            }
            std::cout << "[OK] nested for loops  => 30" << std::endl;
        }

        // 6. Vector literal order: [10,20,30][0] == 10, [...][2] == 30
        //    Locks the visit(VectorLiteral) / NEW_VECTOR ordering fix.
        {
            auto idx0 = std::make_unique<VectorIndex>(vec({10.0, 20.0, 30.0}), num(0.0), 1);
            Value r0 = run_expr(std::move(idx0));
            if (r0.type != ValueType::Number || r0.number_value != 10.0) {
                return fail("[10,20,30][0] == 10", r0) ? 1 : 1;
            }
            auto idx2 = std::make_unique<VectorIndex>(vec({10.0, 20.0, 30.0}), num(2.0), 1);
            Value r2 = run_expr(std::move(idx2));
            if (r2.type != ValueType::Number || r2.number_value != 30.0) {
                return fail("[10,20,30][2] == 30", r2) ? 1 : 1;
            }
            std::cout << "[OK] vector literal order: [10,20,30][0]=10 [2]=30" << std::endl;
        }

        std::cout << "for_loop_e2e_test: all tests passed." << std::endl;
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "for_loop_e2e_test failed: " << ex.what() << std::endl;
        return 1;
    }
}
