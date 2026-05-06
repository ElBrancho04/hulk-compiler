#include <cassert>
#include <iostream>

#include "type_table.hpp"

int main() {
    TypeTable table;

    assert(table.has_type("Object"));
    assert(table.has_type("Number"));
    assert(table.has_type("String"));
    assert(table.has_type("Boolean"));

    table.register_type(TypeInfo{"Foo", "Object"});
    table.register_type(TypeInfo{"Bar", "Foo"});

    assert(table.conforms_to("Bar", "Foo"));
    assert(table.conforms_to("Bar", "Object"));
    assert(!table.conforms_to("Foo", "Bar"));

    assert(table.lowest_common_ancestor("Foo", "Bar") == "Foo");

    const auto vec_number = TypeTable::make_vector_type("Number");
    const auto vec_object = TypeTable::make_vector_type("Object");
    assert(table.conforms_to(vec_number, vec_object));

    const auto iter_number = TypeTable::make_iterable_type("Number");
    const auto iter_object = TypeTable::make_iterable_type("Object");
    assert(table.lowest_common_ancestor(iter_number, iter_object) == iter_object);

    bool caught = false;
    try {
        table.register_type(TypeInfo{"Bad", "Number"});
    } catch (const std::exception&) {
        caught = true;
    }
    assert(caught && "Herencia desde Number debe fallar");

    std::cout << "TypeTable smoke test passed." << std::endl;
    return 0;
}
