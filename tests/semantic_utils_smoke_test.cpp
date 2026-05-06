#include <cassert>
#include <iostream>

#include "semantic_context.hpp"
#include "semantic_error.hpp"
#include "symbol_table.hpp"

int main() {
    SymbolTable table;

    table.define("x", "Number", 1);
    assert(table.lookup("x", 1) == "Number");

    table.enter_scope();
    table.define("y", "String", 2);
    assert(table.lookup("y", 2) == "String");
    assert(table.lookup("x", 2) == "Number");

    table.assign("x", "Object", 3);
    assert(table.lookup("x", 3) == "Object");

    table.exit_scope();

    bool caught = false;
    try {
        table.lookup("y", 4);
    } catch (const SemanticError& err) {
        caught = true;
        assert(err.line() == 4);
    }
    assert(caught);

    SemanticContext ctx;
    ctx.enter_type("Foo");
    ctx.enter_method("bar");
    assert(ctx.in_type);
    assert(ctx.in_method);
    assert(ctx.current_type == "Foo");
    assert(ctx.current_function == "bar");
    ctx.exit_function();
    ctx.exit_type();
    assert(!ctx.in_type);
    assert(!ctx.in_method);

    std::cout << "Semantic utils smoke test passed." << std::endl;
    return 0;
}
